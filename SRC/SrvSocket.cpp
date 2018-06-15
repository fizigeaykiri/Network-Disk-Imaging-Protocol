/**
 * SrvSocket.cpp - Server Socket Class
 *  - Encodes data from the client with sockets using MTCP
 */

#include "SrvSocket.h"

int SrvSocket::server_fd = 0;
int SrvSocket::new_socket = 0;
int SrvSocket::addrlen = 0;
struct sockaddr_in SrvSocket::address;

int SrvSocket::initialize(void)
{
    //int server_fd, new_socket, valread;
	//struct sockaddr_in address;
	//int opt = 1;
	//int addrlen = sizeof(address);

    // Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		return 1;
	}

    address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(SERV_PORT);
	
	// Forcefully attaching socket to the port
	if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
		perror("bind failed");
		return 1;
	}

    return 0;
}

int SrvSocket::serv_listen(void)
{
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
		return 1;
	}

    if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
		perror("accept");
		return 1;
    }

    return 0;
}

int SrvSocket::send_data(uint8_t *send_buffer, int buffer_len)
{
    send(new_socket, send_buffer, buffer_len, 0);

    return 1;
}

int SrvSocket::recv_data(uint8_t *recv_buffer, int buffer_len)
{
    return recv(new_socket, recv_buffer, buffer_len, 0);
}

void SrvSocket::shutdown(void)
{
    close(new_socket);
    close(server_fd);
}

