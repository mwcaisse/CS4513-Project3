#ifndef __NUTELLA_H__
#define __NUTELLA_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/** The maximum length of a movie name */
#define MAX_MOVIE_NAME (255)

/** The maximum length of the movie directory path */
#define MAX_MOVIE_DIRECTORY (255)

/** The maximum length of a host name */
#define MAX_HOST (255)

/** A movie request */ 
#define NUTELLA_REQUEST (1)

/** A response to a movie request */
#define NUTELLA_RESPONSE (2)

/** The request address */
#define REQUEST_ADDR "239.1.3.1"

/** The response address */
#define RESPONSE_ADDR "239.1.3.2"

/** The port for sending requests */
#define REQUEST_PORT 6958

/** The port for sending responses */
#define RESPONSE_PORT 6959

/** The port for streaming movies */
#define STREAM_PORT "6960"

/** The response time out, 15 seconds */
#define RESPONSE_TIME_OUT 15
 

 
/** The structure that represents the message to be sent for requests
	and respones
*/

struct _nutella_msg {
	short type; // the type of request
	char movie_name[MAX_MOVIE_NAME]; // the name of the movie request
	char ip_addr[MAX_HOST]; // the address to initiate the request
	char port[NI_MAXSERV]; // the port to initiate the request
};

typedef struct _nutella_msg nutella_msg_o;

/** Prints the usage of this program
*/

void print_usage();

/** Thread function that will listen for requests to stream movies
*/

void* server(void* arg);

/** Creates a response message and multicasts it out over the specified socekt	
	@param sock The socket to send the message to
	@param movie_name The name of the movie we are responding to
	@return the number of bytes sent or -1 if there was an error
*/

int server_send_response(int sock, char* movie_name);

/** Checks if the server has the specified movie to stream
	@param movie_name The movie to check for
	@return 1 if we have the movie, 0 otherwise
*/

int server_check_movie(char* movie_name);

/** Thread function that will listen for user input, and fetch movies
		to stream
*/

void* client(void* arg);

/** Sends the specified message over the specified socket
	@param sock The socket to send the message to
	@param msg A pointer to the nutella message to send
	@return the number of bytes sent or -1 if there was an error
*/

int nutella_msend(int sock, nutella_msg_o* msg);

/** Creates a request message to request the specified movie for streaming
	@param movie_name A cstring containing the name of the movie requested
	@return A pointer to a nutell_msg_o structure containing the request, that
		should be freed after use
*/

nutella_msg_o* create_request(char* movie_name);

/** Creates a response message to tell a client how to start streaming the 
		requested movie. Contains the ip and port of the client should use to
		initiate the streaming
	@param movie_name The name of the movie that is available to be streamed
	@param addr The address at which the movie can be streamed
	@param port The port at which the movie can be streamed
	@return A pointer to a nutella_msg_o structure containing the response,
		should be freed after use
*/

nutella_msg_o* create_response(char* movie_name, char* addr, char* port);
 
 
#endif
 