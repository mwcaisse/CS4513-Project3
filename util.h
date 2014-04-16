#ifndef __UTIL_H__
#define __UTIL_H__


/** Clears the screen
*/

void clear_screen();

/** Creates a server socket to listen for UDP packets, chooses a random port
		for the socket
	@return The socket descriptor of the created socket
*/

int create_server_socket();

/** Creates a client socket to send and recv data from a server
	@param hostname The hostname of the server to connect to
	@param port The port of the host to connect to
	@return The socket descriptor of the created socket, or -1 if there was an error
*/

int create_client_socket(char* hostname, char* port);


/** Gets the port of the specified socket

	@param sock The socket to get the port of
	@return a string containing the port, that should be freed after use, 
		NULL if there was an error
*/

char* get_sock_port(int sock);

/** Returns a sockaddr struct with the specified host and port name
	@param hostname The hostname to put in the struct
	@param port The port to put in the struct
	@return A pointer to a sockaddr struct, should be freed after use
*/

struct sockaddr* get_sockaddr(char* hostname, char* port);

#endif
