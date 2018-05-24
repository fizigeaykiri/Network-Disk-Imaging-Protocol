/**
 * TestClnt - test client
 * By @massaturn91
 */
 

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h> 

#define PORT	5050

char buffer[1024] = {0}; 
 
int main(int argc, char *argv[])
{
	struct sockaddr_in address;
    struct sockaddr_in serv_addr;
    struct timeval tv;
	int sock = 0;
	int valread;
	
	
	char *hello = "Hello from client";
    char *serv_ip = "127.0.0.1";
	
    tv.tv_sec = 20;
    tv.tv_usec = 0;

    if (argc >= 2) {
        serv_ip = argv[1];
    }

	//buffer = {0};
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}
	
	memset(&serv_addr, '0', sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	// Convert IPv4 & IPv6 addresses from text to binary
	if (inet_pton(AF_INET, serv_ip, &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}
	
	if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv, sizeof(tv)) < 0) {
        printf("Time Out\n");
        return -1;
    }

	send(sock, hello, strlen(hello), 0);
	printf("Hello message sent\n");
	
	do {
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof(tv)) < 0) {
            printf("Time Out\n");
            return -1;
        }

		valread = read(sock, buffer, 1024);
		printf("%s\n", buffer);
		
		fgets(buffer, 1024, stdin);
        printf("Input received\n");

        if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &tv, sizeof(tv)) < 0) {
            printf("Time Out\n");
            return -1;
        }

		send(sock, buffer, strlen(buffer), 0);
		printf("Message sent\n");
		//valread = read(sock, buffer, 1024);
		//printf("%s\n", buffer);
		
	} while (strcmp(buffer, "end\n") != 0);	
	
    close(sock);

	return 0;
}

