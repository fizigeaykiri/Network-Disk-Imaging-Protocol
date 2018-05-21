/**
 * mTcpTstC - MTCP Test Client
 * By @marssaturn91
 */
 
#include <bios.h>
#include <dos.h>
#include <io.h>
#include <stdlib.h>
#include <string.h>

#include "MTCPTSTC.H"

#include "types.h"
#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "udp.h"
#include "dns.h"
#include "tcp.h"
#include "tcpsockm.h"

//static void setup_test(void);
static void parse_args(int argc, char *argv[]);
static void shutdown(int rc);

// Global vars and flags

char     ServerAddrName[80];     // Server name
uint16_t ServerPort;             // Only used when we are a client.
uint16_t LclPort = 2048;         // Local port to use for our socket (0 means not set)

int8_t   Listening = -1;         // Are we the server (1) or the client (0)?


#define RECV_BUFFER_SIZE (1024)
uint8_t recvBuffer[ RECV_BUFFER_SIZE ];


// Trap Ctrl-Break and Ctrl-C to that we can unhook the timer interrupt
//  and shutdown cleanly.

// Check this flag once in a while to see if the user wants out.
volatile uint8_t CtrlBreakDetected = 0;

void (__interrupt __far *oldCtrlBreakHandler) ();

void __interrupt __far ctrlBreakHandler() 
{
    CtrlBreakDetected = 1;
}

void __interrupt __far ctrlCHandler() 
{
    // Do Nothing
}


int main(int argc, char *argv[])
{
    printf("mTcpTstC - MTCP Test Client\n");
    
    // Read command line arguments
    parse_args(argc, argv);
    
    // Setup mTCP environment
    if (Utils::parseEnv() != 0) {
        exit(-1);
    }
    
    // Initialize TCP/IP Stack
    if (Utils::initStack(2, TCP_SOCKET_RING_SIZE)) {
        fprintf(stderr, "\nFailed to initialize TCP/IP - Make sure your Packet Driver is loaded - exiting\n");
        exit(-1);
    }
    
    // Save off the oldCtrlBreakHandler and put our own in. Shutdown() 
    //  will restore the original handler for us.
    
    oldCtrlBreakHandler = getvect(0x1B);
    setvect(0x1B, ctrlBreakHandler);
    
    // Get the Ctrl-C interrupt too, but do nothing. We actually want Ctrl-C
    //  to be a legal character to send when in interactive mode.
    setvect(0x23, ctrlCHandler);
    
    
    TcpSocket *mySocket;    
    int8_t rc;
    
    if (Listening == 0) {
        printf("Resolving server address - press Ctrl-Break to abort.\n\n");
        
        IpAddr_t serverAddr;
        
        // Resolve this name and definitely send the request
        int8_t rc2 = Dns::resolve(ServerAddrName, serverAddr, 1);
        if (rc2 < 0) {
            fprintf(stderr, "Error resolving server\n");
            shutdown(-1);
        }
        
        uint8_t done = 0;
        
        while (!done) {
            if (CtrlBreakDetected) {
                break;
            }
            
            if (!Dns::isQueryPending()) {
                break;
            }
            
            PACKET_PROCESS_SINGLE;
            Arp::driveArp();
            Tcp::drivePackets();
            Dns::drivePendingQuery();
        }
        
        // Query is no longer pending or we bailed out of the loop.
        rc2 = Dns::resolve(ServerAddrName, serverAddr, 0);
        if (rc2 != 0) {
            fprintf(stderr, "Error resolving server\n");
            shutdown(-1);
        }
        
        mySocket = TcpSocketMgr::getSocket();
        mySocket->setRecvBuffer(RECV_BUFFER_SIZE);
        
        printf("Server resolved to %d.%d.%d.%d - connecting\n\n", 
            serverAddr[0], serverAddr[1], serverAddr[2], serverAddr[3]);
        
        // Non-blocking connect. Wait 10 seconds before giving up.
        rc = mySocket->connect(LclPort, serverAddr, ServerPort, 10000);
    } else {  
        // This is the server code - not really needed now
        /*
        printf("Waiting for a connection on port %u. Press [ESC] to abort.\n\n", LclPort);
        
        TcpSocket *listeningSocket = TcpSocketMgr::getSocket();
        listeningSocket->listen(LclPort, RECV_BUFFER_SIZE);
        
        // Listen is Non-blocking. Need to wait
        while (1) {
            if (CtrlBreakDetected) {
                rc = -1;
                break;
            }
            
            PACKET_PROCESS_SINGLE;
            Arp::driveArp();
            Tcp::drivePackets();
            
            mySocket = TcpSocketMgr::accept();
            if (mySocket != NULL) {
                listeningPort->close();
                TcpSocketMgr::freeSocket(listeningSocket);
                rc = 0;
            }
            
            if (_bios_keybrd(1) != 0) {
                char c _bios_keybrd(0);
                if (c == 27 || c ==3) {
                    rc = -1;
                    break;
                }
            }
        }
        */
    }
    
    if (rc != 0) {
        fprintf(stderr, "Socket open failed\n");
        shutdown(-1);
    }

    if (Listening == 0) {
        fprintf(stderr, "Connected!\n\n");
    }
    
    uint8_t done = 0;
    
    while (!done) {
        if (CtrlBreakDetected) {
            printf("\nCtrl-Break detected\n");
            done = 1;
        }
        
        // Service the connection
        PACKET_PROCESS_SINGLE;
        Arp::driveArp();
        Tcp::drivePackets();
        
        if (mySocket->isRemoteClosed()) {
            done = 1;
        }
        
        // Process incoming packets first.
        int16_t recvRc = mySocket->recv(recvBuffer, RECV_BUFFER_SIZE);
        
        if (recvRc > 0) {
            write(1, recvBuffer, recvRc);
        }
        
        if (_bios_keybrd(1)) {
            uint16_t key = _bios_keybrd(0);
            char ch = key & 0xFF;
            
            if (ch == 0) {
                uint8_t ekey = key >> 8;
                
                if (ekey == 45) {   // Alt-X
                    done = 1;
                }
                else if (ekey == 35) {  // Alt-H
                    printf("\nMTCPTSTC: Press Alt-X to exit\n\n");
                }
            } else {
                int8_t sendRc = mySocket->send((uint8_t *)&ch, 1);
                if (sendRc < 1) {
                    fprintf(stderr, "Unable to send packet\n");
                }
            }
        }
    }
    
    mySocket->close();
    TcpSocketMgr::freeSocket(mySocket);
    shutdown(0);
    
    return 0;    
}

static void parse_args(int argc, char *argv[])
{
    Listening = 0;
    ServerPort = 5050;
    
    if (argc != 2) {
        printf("Usage: MTCPTSTC <ipaddr|dnsdomain>\n");
        exit(1);
    }
    
    strcpy(ServerAddrName, argv[1]);
    
    //for (int i = 1; i < argc; i++) {
    //    strcpy(ServerAddrName, argv[i]);
    //}
}

static void shutdown(int rc) 
{
    setvect(0x1B, oldCtrlBreakHandler);

    Utils::endStack();
    Utils::dumpStats(stderr);
    fclose(TrcStream);
    exit(rc);
}    


