
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <readline/readline.h>

#include "nutella.h"
#include "util.h"
#include "msock.h"

/** The pthread that the server is running in */
pthread_t server_thread;
/** The pthread that the client is running in */
pthread_t client_thread;

int main(int argc, char* argv[]) {

	//check if any arguments were returned
	if (argc != 1) {
		printf("nutella does not take arguments \n");
		return 0;
	}
	
	
	
	pthread_create(&server_thread, NULL, &server, NULL);
	pthread_create(&client_thread, NULL, &client, NULL);
	
	// wait for the server + client threads to terminate
	
	pthread_join(server_thread, NULL);
	pthread_join(client_thread, NULL);

}


void* server(void* arg) {

	//we recevice requests, and send responses
	int sock_recv = msockcreate(RECV, REQUEST_ADDR, REQUEST_PORT);
	int sock_send = msockcreate(SEND, RESPONSE_ADDR, RESPONSE_PORT);
	
	if (sock_recv < 0 || sock_send < 0) {
		printf("Unable to start server, could not create sockets \n");		
		return NULL;
	}
	
	int running = 1;
	
	while (running) {
		
		nutella_msg_o msg;
		
		int res = mrecv(sock_recv, (char*)&msg, sizeof(msg), 0);	
		
		if (res == 1) {
			perror("couldn't read msg");
		}
		else {
			printf("received msg: type %d, movie_name %s \n", msg.type, msg.movie_name);
			
			nutella_msg_o* resp = create_response(msg.movie_name, "192.168.1.160", 
				STREAM_PORT);	
				
			nutella_msend(sock_send, resp);
			free(resp);
	
		}
		
	}
	

}

void* client(void* arg) {

	// we send requests, and receive responses
	int sock_send = msockcreate(SEND, REQUEST_ADDR, REQUEST_PORT);
	int sock_recv = msockcreate(RECV, RESPONSE_ADDR, RESPONSE_PORT);
	
	if (sock_recv < 0 || sock_send < 0) {
		printf("Unable to start server, could not create sockets \n");		
		return NULL;
	}
	
	int running = 1;
	
	while (running) {
		char* movie_name = readline("Enter movie name:");
		nutella_msg_o* msg = create_request(movie_name);
		
		nutella_msend(sock_send, msg);
		free(msg);
		
		nutella_msg_o buf;
		int res = mrecv(sock_recv, (char*)&buf, sizeof(buf), 0);	
		if (res == 1) {
			perror("couldn't read msg");
		}
		else {
			printf("Sreceived msg: type %d, movie_name %s, ip %s, port %s \n", 
				buf.type, buf.movie_name, buf.ip_addr, buf.port);
		
		}
		
	}

}

/** Sends the specified message over the specified socket
	@param sock The socket to send the message to
	@param msg A pointer to the nutella message to send
	@return the number of bytes sent or -1 if there was an error
*/

int nutella_msend(int sock, nutella_msg_o* msg) {
	return msend(sock, (char*)msg, sizeof(nutella_msg_o));
}


/** Creates a request message to request the specified movie for streaming
	@param movie_name A cstring containing the name of the movie requested
	@return A pointer to a nutell_msg_o structure containing the request, that
		should be freed after use
*/

nutella_msg_o* create_request(char* movie_name) {
	nutella_msg_o* request = (nutella_msg_o*) malloc( sizeof(nutella_msg_o));
	
	request->type = NUTELLA_REQUEST;
	//copy over the movie name, and zero out the other strings
	strncpy(request->movie_name, movie_name, MAX_MOVIE_NAME);
	memset(request->ip_addr, 0, NI_MAXHOST);
	memset(request->port, 0, NI_MAXSERV);
	
	return request;
}

/** Creates a response message to tell a client how to start streaming the 
		requested movie. Contains the ip and port of the client should use to
		initiate the streaming
	@param movie_name The name of the movie that is available to be streamed
	@param addr The address at which the movie can be streamed
	@param port The port at which the movie can be streamed
	@return A pointer to a nutella_msg_o structure containing the response,
		should be freed after use
*/

nutella_msg_o* create_response(char* movie_name, char* addr, char* port) {
	nutella_msg_o* response = (nutella_msg_o*) malloc( sizeof(nutella_msg_o));
	
	response->type = NUTELLA_RESPONSE;
	
	strncpy(response->movie_name, movie_name, MAX_MOVIE_NAME);
	strncpy(response->ip_addr, addr, NI_MAXHOST);
	strncpy(response->port, port, NI_MAXSERV);
	
	return response;
}


