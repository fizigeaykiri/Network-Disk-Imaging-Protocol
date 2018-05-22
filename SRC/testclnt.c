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

char buffer[1024]; 
 
int main(int argc, char *argv[])
{
	struct sockaddr_in address;
	int sock = 0;
	int valread;
	struct sockaddr_in serv_addr;
	
	char *hello = "Hello from client";
	
	buffer = {0};
	
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}
	
	memset(&serv_addr, '0', sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	
	// Convert IPv4 & IPv6 addresses from text to binary
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}
	
	if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("\nConnection Failed \n");
		return -1;
	}
	
	send(sock, hello, strlen(hello), 0);
	printf("Hello message sent\n");
	
	do {
		valread = read(sock, buffer, 1024);
		printf("%s\n", buffer);
		
		fgets(buffer, 1024, stdin);
		send(sock, buffer, strlen(buffer), 0);
		printf("Message sent\n");
		//valread = read(sock, buffer, 1024);
		//printf("%s\n", buffer);
		
	} (while (strcmp(buffer, "end") != 0));	
	
	return 0;
}

