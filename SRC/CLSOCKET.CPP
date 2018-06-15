/**
 * CLSOCKET.CPP - Client Socket Class
 *  - Encodes data from the client with sockets using MTCP
 */

#include "CLSOCKET.H"

extern volatile uint8_t CtrlBreakDetected;


char CLSOCKET::servAddrName[80] = {0};
uint16_t CLSOCKET::servPort = SERV_PORT;
uint16_t CLSOCKET::localPort = LCL_PORT;
TcpSocket* CLSOCKET::mySocket;

int CLSOCKET::initialize(void)
{
    uint8_t foobar = 20;
    
    printf("CLSOCKET::initialize()\n");
    
    // Setup mTCP environment
    if (Utils::parseEnv() != 0) {
        return -1;
    }

    // Initialize TCP/IP Stack
    if (Utils::initStack(2, TCP_SOCKET_RING_SIZE)) {
        fprintf(stderr, "\nFailed to initialize TCP/IP - Make sure your Packet Driver is loaded - exiting\n");
        return -1;
    }

    return 0;
}

/*
int CLSOCKET::connect_server(void)
{
    //TODO: Figure Out Later
}
*/

int CLSOCKET::connect_server(char* nservAddrName)
{    
    return connect_server(nservAddrName, SERV_PORT);
}

int CLSOCKET::connect_server(char* nservAddrName, uint16_t servPort)
{
    IpAddr_t serverAddr;
    int8_t rc;

    printf("CLSOCKET::connect_server()\n");
    
    // Resolve this name and definitely send the request
    int8_t rc2 = Dns::resolve(nservAddrName, serverAddr, 1);
    if (rc2 < 0) {
        fprintf(stderr, "Error resolving server\n");
        shutdown();
        return -1;
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
    rc2 = Dns::resolve(nservAddrName, serverAddr, 0);
    if (rc2 != 0) {
        fprintf(stderr, "Error resolving server\n");
        shutdown();
        return -1;
    }
        
    mySocket = TcpSocketMgr::getSocket();
    mySocket->setRecvBuffer(BUFFER_SIZE);
    
    printf("Server resolved to %d.%d.%d.%d - connecting\n\n", 
        serverAddr[0], serverAddr[1], serverAddr[2], serverAddr[3]);
    
    // Non-blocking connect. Wait 10 seconds before giving up.
    rc = mySocket->connect(localPort, serverAddr, servPort, 10000);

    if (rc != 0) {
        fprintf(stderr, "Socket open failed, couldn't connect to server.\n");
        shutdown();
        return -1;
    }

    printf("Connection to server successful.\n");

    return 0;
}

int CLSOCKET::update_connection(void)
{
    // TODO: Detect Innterupt

    // Service the connection
    PACKET_PROCESS_SINGLE;
    Arp::driveArp();
    Tcp::drivePackets();

    if (mySocket->isRemoteClosed()) {
        return 1;
    }

    return 0;
}

int8_t CLSOCKET::send_data(uint8_t *sendBuffer, int char_count)
{
    printf("CLSOCKET::send_data()\n");
    
    int8_t sendRc = mySocket->send(sendBuffer, char_count);

    return sendRc;
}

int16_t CLSOCKET::recv_data(uint8_t *recvBuffer, int16_t recv_buffer_size)
{
    if (recvBuffer == NULL) {
        fprintf(stderr, "Null Pointer passed into CLSOCKET::recv_data().\n");
        return -1;
    }

    printf("CLSOCKET::recv_data()\n");
    
    //int16_t recvRc = mySocket->recv(recvBuffer, RECV_BUFFER_SIZE);
    int16_t recvRc = mySocket->recv(recvBuffer, recv_buffer_size);

    /*
    if (recvRc > 0) {
        return recvBuffer;
    } else {
        return NULL;
    }
    */

    return recvRc;
}

void CLSOCKET::close_connection(void)
{
    printf("CLSOCKET::close_connection\n");
    
    mySocket->close();
    TcpSocketMgr::freeSocket(mySocket);
    shutdown();
}

void CLSOCKET::shutdown(void)
{
    printf("CLSOCKET::shutdown\n");
    
    Utils::endStack();
    Utils::dumpStats(stderr);
    fclose(TrcStream);
}