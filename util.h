#ifndef __UTIL_H__
#define __UTIL_H__


/** Clears the screen
*/

void clear_screen();

/** Creates a server socket to listen for UDP packets
	@param port The port to listen for data on
	@return The socket descriptor of the created socket
*/

int create_server_socket(char* port);

/** Creates a client socket to send and recv data from a server
	@param hostname The hostname of the server to connect to
	@param port The port of the host to connect to
	@return The socket descriptor of the created socket, or -1 if there was an error
*/

int create_client_socket(char* hostname, char* port);

#endif
