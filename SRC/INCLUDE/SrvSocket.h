/**
 * SrvSocket.h - Server Socket Class Header
 *  - decodes data using sockets for the server
 */

#ifndef _SRVSOCKET_H_
#define _SRVSOCKET_H_

#include "NDIPTYPE.H"

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>

#define BUFFER_SIZE     1500
#define SERV_PORT       5050


class SrvSocket
{
    public:
        static int initialize(void);
        static int serv_listen(void);
        static int send_data(uint8_t *send_buffer, int buffer_len);
        static int recv_data(uint8_t *recv_buffer, int buffer_len);
        static void shutdown(void);

    private:
        static int server_fd;
        static int new_socket;
        static int addrlen;
        static struct sockaddr_in address;

        SrvSocket(void);
};

#endif

