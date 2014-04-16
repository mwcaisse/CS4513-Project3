
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "util.h"


/** Clears the screen
*/

void clear_screen() {
  printf("\033[2J");
  printf("\033[0;0f");
}


/** Creates a server socket to listen for UDP packets, chooses a random port
		for the socket
	@param port the port to bind to
	@return The socket descriptor of the created socket
*/

int create_server_socket(int port) {

	int sockfd = -1;
	int res = -1;
	
	struct sockaddr_in addr_me;
	
	sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	//check if we created a socket
	if (sockfd < 0) {
		perror("Unable to create socket");
		return sockfd;
	}
	
	int yes = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
		perror("UTIL: couldnt set sock opt REUSEADDR");
		close(sockfd);
	}
	
	
	memset(&addr_me, 0, sizeof(addr_me));
	addr_me.sin_family = AF_INET;
	addr_me.sin_port = htons(9874);
	addr_me.sin_addr.s_addr = htonl(INADDR_ANY); // my up
	
	res = bind(sockfd, (struct sockaddr *)&addr_me, sizeof(addr_me));
	
	if (res) {
		perror("Unable to bind");
		close(sockfd);
		return -1;
	}
	
	get_sock_port(sockfd);
	
	return sockfd;	

}

/** Returns a sockaddr struct with the specified host and port name
	@param hostname The hostname to put in the struct
	@param port The port to put in the struct
	@return A pointer to a sockaddr struct, should be freed after use
*/

struct sockaddr* get_sockaddr(char* hostname, char* port) {
	struct sockaddr_in* addr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
	
	unsigned int portui;
	sscanf(port, "%d", &portui);
	
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(portui);
	addr->sin_addr.s_addr = inet_addr(hostname);

	return (struct sockaddr*) addr;
	
}

/** Gets the port of the specified socket

	@param sock The socket to get the port of
	@return a string containing the port, that should be freed after use, 
		NULL if there was an error
*/

char* get_sock_port(int sock) {	
	struct sockaddr_in addr;
	socklen_t size;
	
	if (getsockname(sock, (struct sockaddr*) &addr, &size)) {
		perror("Couldnt get port");
		return NULL;
	}
	
	char* port = (char*) malloc(NI_MAXSERV);
	
	printf("RAW PORT: %d\n", addr.sin_port);
	
	snprintf(port, NI_MAXSERV, "%d", ntohs(addr.sin_port));
	
	return port;
}

/*
int create_server_socket(char* port) {
	int sockfd = -1;
	int res = -1;
	struct addrinfo hints;
	struct addrinfo* server_info;
	struct addrinfo* ptr;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	if ((res = getaddrinfo(NULL, port, &hints, &server_info)) != 0) {
		perror("Error getting address info");
		return -1;
	}
	
	//find a good socket
	for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
		if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
			perror("Couldn't create socket");
			continue;
		}
		
		if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			close(sockfd);
			sockfd = -1;
			perror("Couldn't bind");
			continue;
		}
		
		//woo we have created a socket and bound to it, or ran out of sockets
		break;
	}
	
	//free address info
	freeaddrinfo(server_info);
	
	//return the socket we created, or -1 if we didnt create a socket
	return sockfd;
}*/

/** Creates a client socket to send and recv data from a server
	@param hostname The hostname of the server to connect to
	@param port The port of the host to connect to
	@return The socket descriptor of the created socket, or -1 if there was an error
*/

int create_client_socket(char* hostname, char* port) {

	int sockfd = -1;
	int res = -1;
	
	struct addrinfo hints;
	struct addrinfo* server_info;
	struct addrinfo* ptr;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	
	if ((res = getaddrinfo(hostname, port, &hints, &server_info))) {
		perror("Couldnt get server address info");
		return -1;
	}
	
	for (ptr = server_info; ptr != NULL; ptr = ptr->ai_next) {
		if ((sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0)  {
			perror("Couldnt create socket");
			continue;
		}
		
		break;
	}
	
	freeaddrinfo(server_info);
	
	if (ptr == NULL) {
		perror("Couldnt find a socket to create");
		return -1;
	}
	
	return sockfd;

}
