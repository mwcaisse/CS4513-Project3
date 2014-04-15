
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


/** The IP address of this nutella client */
char* my_ip;

/** The movie directory that contains the movies to stream */
char* movie_directory;

int main(int argc, char* argv[]) {



	///check arguments
	if (argc != 3) {
		print_usage();
		return 0;
	}
	
	my_ip = (char*) malloc(MAX_HOST + 1);
	movie_directory = (char*) malloc(MAX_MOVIE_DIRECTORY + 1);
	
	//copy over the arguments
	strncpy(my_ip, argv[1], MAX_HOST);
	strncpy(movie_directory, argv[2], MAX_MOVIE_DIRECTORY);
	
	
	
	pthread_create(&server_thread, NULL, &server, NULL);
	pthread_create(&client_thread, NULL, &client, NULL);
	
	// wait for the server + client threads to terminate
	
	pthread_join(server_thread, NULL);
	pthread_join(client_thread, NULL);

}

/** Prints the usage of this program
*/

void print_usage() {
	printf("Usage: \n");
	printf("\t ./nutella ipaddress moviedir \n");
	printf("Example: \n");
	printf("\t ./nutella 192.168.1.160 ./movies/ \n");
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
			continue;
		}

		//we have received a msg.
		if (msg.type != NUTELLA_REQUEST) {
			//we are only listening for requests,
			//some client is being malicious yay
			printf("Malicious client detected \n");
			continue;
		}
		printf("received msg: type %d, movie_name %s \n", msg.type, msg.movie_name);
		
		//check if we have the movie		
		if(!server_check_movie(msg.movie_name)) {
			//we don't have the movie, dont do anything
			printf("Movie %s not found \n", msg.movie_name);
			continue;
		}

		//woo we have the movie
		printf("We have the movie sending the response \n");
		server_send_response(sock_send, msg.movie_name);

		
	}
	

}

/** Creates a response message and multicasts it out over the specified socekt	
	@param sock The socket to send the message to
	@param movie_name The name of the movie we are responding to
	@return the number of bytes sent or -1 if there was an error
*/

int server_send_response(int sock, char* movie_name) {
	nutella_msg_o* resp = create_response(movie_name, my_ip, STREAM_PORT);	
	int res = nutella_msend(sock, resp);
	free(resp);
	return res;
}

/** Checks if the server has the specified movie to stream
	@param movie_name The movie to check for
	@return 1 if we have the movie, 0 otherwise
*/

int server_check_movie(char* movie_name) {
	char* file_name = (char*) malloc(MAX_MOVIE_DIRECTORY + MAX_MOVIE_NAME + 2);
	strncpy(file_name, movie_directory, MAX_MOVIE_DIRECTORY);
	strncat(file_name, "/", 2);
	strncat(file_name, movie_name, MAX_MOVIE_NAME);
	if (!access(file_name, F_OK)) {
		return 1; // file exists, we must have the movie
	}
	return 0; //file does not exist we do not have movie
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
		
		//lets wait for a response
		int waiting = RESPONSE_TIME_OUT;
		int res = 0;
		
		while (waiting) {
			res = mrecv(sock_recv, (char*)&buf, sizeof(buf), MSG_DONTWAIT);	
			if (res < 0 && errno == EAGAIN) {
				printf("No data, waiting \n");
				waiting --; // decrement the timeout counter
				sleep(1); // sleep for a second				
			}
			else {	
				break;
			}
			
		}
		if (res < 0) {
			if (errno != EAGAIN) {
				perror("error receiving message");
			}
			else {
				printf("Timed out. No one has the movie %s \n", movie_name);
			}
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
	memset(request->ip_addr, 0, MAX_HOST);
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
	strncpy(response->ip_addr, addr, MAX_HOST);
	strncpy(response->port, port, NI_MAXSERV);
	
	return response;
}


