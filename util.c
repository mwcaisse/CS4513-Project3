
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


/** Creates a server socket to listen for UDP packets
	@param port The port to listen for data on
	@return The socket descriptor of the created socket
*/

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
}

/** Creates a client socket to send and recv data from a server
	@param hostname The hostname of the server to connect to
	@param port The port of the host to connect to
	@return The socket descriptor of the created socket, or -1 if there was an error
*/

int create_client_socket(char* hostname, char* port) {

}
