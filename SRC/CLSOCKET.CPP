/**
 * CLSOCKET.CPP - Client Socket Class
 *  - Encodes data from the client with sockets using MTCP
 */

#include "CLSOCKET.CPP"

extern volatile uint8_t CtrlBreakDetected;


int CLSOCKET::initialize(void)
{
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

    // Resolve this name and definitely send the request
    int8_t rc2 = Dns::resolve(ServerAddrName, serverAddr, 1);
    if (rc2 < 0) {
        fprintf(stderr, "Error resolving server\n");
        shutdown(-1);
        return -1;
    }

    uint8_t done = 0;

    while (!done) {
        //TODO: Add Interrupt

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
        return -1;
    }
        
    mySocket = TcpSocketMgr::getSocket();
    mySocket->setRecvBuffer(BUFFER_SIZE);
    
    printf("Server resolved to %d.%d.%d.%d - connecting\n\n", 
        serverAddr[0], serverAddr[1], serverAddr[2], serverAddr[3]);
    
    // Non-blocking connect. Wait 10 seconds before giving up.
    rc = mySocket->connect(LclPort, serverAddr, ServerPort, 10000);

    if (rc != 0) {
        fprintf(stderr, "Socket open failed, couldn't connect to server.\n");
        shutdown(-1);
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

int8_t CLSOCKET::send_data(uint8_t *sendBuffer)
{
    int8_t sendRc = mySocket->send(sendBuffer, char_count);

    return sendRc;
}

uint8_t* CLSOCKET::recv_data()
{
    int16_t recvRc = mySocket->recv(recvBuffer, RECV_BUFFER_SIZE);

    return recvBuffer;
}

void CLSOCKET::close_connection(void)
{
    mySocket->close();
    TcpSocketMgr::freeSocket(mySocket);
    shutdown();
}

void CLSOCKET::shutdown(void)
{
    Utils::endStack();
    Utils::dumpStats(stderr);
    fclose(TrcStream);
}