
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

/** The move that the user has last requested */
char* requested_movie;

/** The stream id counter */
int stream_id = 1;

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
		//printf("SERV: received msg: type %d, movie_name %s \n", msg.type, msg.movie_name);
		
		//check if we have the movie		
		if(!server_check_movie(msg.movie_name)) {
			//we don't have the movie, dont do anything
			printf("SERV: Movie %s not found \n", msg.movie_name);
			continue;
		}

		//woo we have the movie
		//printf("We have the movie sending the response \n");
		server_listen_stream(sock_send, msg.movie_name);

		
	}
	

}

/** Creates a response message and multicasts it out over the specified socekt	
	@param sock The socket to send the message to
	@param movie_name The name of the movie we are responding to
	@oaram port The port to tell the client we are listening on
	@return the number of bytes sent or -1 if there was an error
*/

int server_send_response(int sock, char* movie_name, char* port) {
	nutella_msg_o* resp = create_response(movie_name, my_ip, port);	
	int res = nutella_msend(sock, resp);
	free(resp);
	return res;
}

/** Checks if the server has the specified movie to stream
	@param movie_name The movie to check for
	@return 1 if we have the movie, 0 otherwise
*/

int server_check_movie(char* movie_name) {
	char* file_name = get_movie_path(movie_name);
	if (!access(file_name, F_OK)) {
		free(file_name);
		return 1; // file exists, we must have the movie
	}
	free(file_name);
	return 0; //file does not exist we do not have movie
}

/** Gets the path to the movie
	@param movie_name THe name of the movie
	@param A string containing the path to the movie
*/

char* get_movie_path(char* movie_name) {
	char* file_name = (char*) malloc(MAX_MOVIE_DIRECTORY + MAX_MOVIE_NAME + 2);
	strncpy(file_name, movie_directory, MAX_MOVIE_DIRECTORY);
	strncat(file_name, "/", 2);
	strncat(file_name, movie_name, MAX_MOVIE_NAME);
	return file_name;
}

/** Creates a socket to listen for connections from a client to stream the specified movie.
	Will timeout if it does not hear from a client after the STREAM_TIMEOUT time
	@param notify_sock The id of the socket to send the request to the client to notify that we
		have the file and how to connect
	@param movie_name The name of the movie to stream
	@return 0 if sucessful, 1 if timed out, -1 if error
*/

int server_listen_stream(int notify_sock, char* movie_name) {
	int listen_sock = create_server_socket(STREAM_PORT);
	
	if (listen_sock < 0) {
		//we couldn't create a socket,
		return -1;
	}
	
	char* port = get_sock_port(listen_sock);
	if (port == NULL) {
		//error retreiving the port, cant continue without it
		return -1;
	}
	//printf("SERV: PORT: %s \n", port);
	//we have our socket, lets notify the user
	if (server_send_response(notify_sock, movie_name, port) < 0) {
		perror("SERV: Couldn't notfy client of movie");
		close(listen_sock);
		free(port);
		return -1;
	}
	
	//we are done with the port, free it
	free(port);
	
	//we have the socket set up, listen for requests from the client

	//the struct to contain the address of our client
	//we will just stream the movie to the first client to respond
	struct sockaddr addr_client;
	socklen_t addr_len;
	// the buffer for receiving messages
	nutella_msg_o buf;
	
	int waiting = STREAM_TIMEOUT;
	int res = 0;
	
	while (waiting) {
		res = recvfrom(listen_sock, &buf, sizeof(buf), MSG_DONTWAIT,
			&addr_client, &addr_len);
			
		if (res < 0 && errno == EAGAIN) {
			//no data
			waiting --; // decrement the wait counter
			sleep(1); // sleep for a second
		}
		else if (buf.type != NUTELLA_STREAM_START || 
			strncmp(buf.movie_name, movie_name, MAX_MOVIE_NAME) !=0) {
			
			//request to stream a different movie, or not a stream start message
		}
		else {
			break; // stream start message was received
		}
	}
	
	//check if we tiemd out
	if (res < 0) {
		if (errno != EAGAIN) {
			perror("SERV: error receiving message");
			close(listen_sock);
			return -1;
		}
		else {
			printf("SERV: Streaming timed out, no client responded \n");		
			close(listen_sock);
			return 1;
		}
	}
	
	//we didnt time out, time to start streaming

	//open the file.
	char* file_name = get_movie_path(movie_name);
	FILE* file = fopen(file_name, "r");
	free(file_name);
	
	if (!file) {
		perror("SERV: Couldnt open file");
		close(listen_sock);
		//also send out the movie over message, and stop the stream
		return -1;
	}
	
	//start sending the file to the stream line by line
	
	char* line_buffer = malloc(MAX_STREAM_DATA);
	line_buffer[0] = '\0';
	int data_count = MAX_STREAM_DATA;
	
	char* line = NULL;
	size_t len = 0;
	
	int cur_frame = 0;
	int sleep_time = (1000 / STREAM_FPS) * 1000;
	int stream_id = get_next_stream_id();
	int read = 0;
	
	while ( (read = getline(&line, &len, file)) != -1) {
		if (strncmp(line, "end", 3) == 0) {
			
			if (data_count < MAX_STREAM_DATA) {
				//data buffer has data in it, send the message
				stream_msg_o* msg = create_stream_msg(stream_id, cur_frame,
					 0, line_buffer);
					 
				res = sendto(listen_sock, msg, sizeof(stream_msg_o), 0,
					&addr_client, addr_len);
				if (res < 0) {
					perror("SERV: sending stream message 1");
				}
				free(msg);
				
				memset(line_buffer, 0, MAX_STREAM_DATA);
				line_buffer[0] = '\0';
				data_count = MAX_STREAM_DATA;
			}
		
			usleep(sleep_time);
			cur_frame ++;	
			//free(line);
			continue;		
		}
		
		if (read > data_count) {
			//clean out the buffer
			stream_msg_o* msg = create_stream_msg(stream_id, cur_frame,
				0, line_buffer);
				 
			res = sendto(listen_sock, msg, sizeof(stream_msg_o), 0,
				&addr_client, addr_len);
			if (res < 0) {
				perror("SERV: sending stream message 2");
			}
			free(msg);
			
			memset(line_buffer, 0, MAX_STREAM_DATA);
			line_buffer[0] = '\0';
			data_count = MAX_STREAM_DATA;
		}

		//append the data to the end of the buffer
		strncat(line_buffer, line, read);
		data_count -= read; 
		
		//free(line);
		
	}
	
	//send the finish message to the client
	stream_msg_o* msg = create_stream_msg(stream_id, -1,
		1, line_buffer);
	res = sendto(listen_sock, msg, sizeof(stream_msg_o), 0,
		&addr_client, addr_len);
		
	if (res < 0) {
		perror("SERV: sending stream message 3");
	}
	
	free(msg);
	free(line_buffer);
	
	return 0;
}



void* client(void* arg) {

	// we send requests, and receive responses
	int sock_send = msockcreate(SEND, REQUEST_ADDR, REQUEST_PORT);
	int sock_recv = msockcreate(RECV, RESPONSE_ADDR, RESPONSE_PORT);
	
	if (sock_recv < 0 || sock_send < 0) {
		printf("CLIENT: Unable to start server, could not create sockets \n");		
		return NULL;
	}
	
	int running = 1;
	
	while (running) {
		printf("Enter movie name: ");
		char* movie_name = (char*) malloc(MAX_MOVIE_NAME);
		scanf("%s", movie_name);
		nutella_msg_o* msg = create_request(movie_name);
		
		nutella_msend(sock_send, msg);
		free(msg);
		
		nutella_msg_o buf;
		
		//lets wait for a response
		int waiting = RESPONSE_TIMEOUT;
		int res = 0;
		
		while (waiting) {
			res = mrecv(sock_recv, (char*)&buf, sizeof(buf), MSG_DONTWAIT);	
			if (res < 0 && errno == EAGAIN) {
				//printf("No data, waiting \n");
				waiting --; // decrement the timeout counter
				sleep(1); // sleep for a second				
			}
			else if (strncmp(buf.movie_name, movie_name, MAX_MOVIE_NAME) != 0) {	
				//found a movie with a different name, discard it.
			}
			else {
				break; // we found a movie, and it has the correct name
			}			
			
		}
		if (res < 0) {
			if (errno != EAGAIN) {
				perror("CLIENT: error receiving message");
			}
			else {
				printf("CLIENT: Timed out. No one has the movie %s \n", movie_name);
			}
		}
		else {
			//printf("CLIENT: received msg: type %d, movie_name %s, ip %s, port %s \n", 
				//buf.type, buf.movie_name, buf.ip_addr, buf.port);
				
			client_stream_movie(&buf);
		
		}
		
		free(movie_name);
		
		
		
	}

}

/** Starts streaming the movie from the server specified in the msg
	@param msg The msg received from the server streaming the movie
	@return 0 if sucessful, 1 if timedout, -1 if error
*/

int client_stream_movie(nutella_msg_o* msg) {

	int sockfd = create_client_socket(msg->ip_addr, msg->port);
	
	if (sockfd < 0) {
		return -1;
	}
	
	nutella_msg_o msg_stream;
	msg_stream.type = NUTELLA_STREAM_START;
	strncpy(msg_stream.movie_name, msg->movie_name, MAX_MOVIE_NAME);
	
	struct sockaddr* addr_server = get_sockaddr(msg->ip_addr, msg->port);
	
	int res = sendto(sockfd, &msg_stream, sizeof(msg_stream), 0, addr_server,
		(socklen_t)sizeof(struct sockaddr));
	
	free(addr_server);
		
	if (res < 0) {
		//couldnt send message to start the stream
		perror("CLIENT: Sending stream message");
		close(sockfd);
		return -1;
	}
	
	
	struct sockaddr addr_from;
	socklen_t addr_len;
	
	int waiting = STREAM_TIMEOUT;	
	int cur_frame = -1;
	
	stream_msg_o buf;
	
	while (waiting) {
		res = recvfrom(sockfd, &buf, sizeof(buf), MSG_DONTWAIT, 
			&addr_from, &addr_len);
			
		if (res < 0 && errno == EAGAIN) {
			//no data, available
			waiting --;
			sleep(1);
		}
		else {
			//there is data available, reset the time out
			waiting = STREAM_TIMEOUT;
			
			if (buf.done) {
				//movie is done, clear screen and break
				clear_screen();
				break;
			}
			
			//check if we have advanced a frame
			if (buf.frame != cur_frame) {
				cur_frame = buf.frame;
				clear_screen();
			}
			
			printf("%s", buf.data);
		}
	}
	
	//printf("CLIENT: Movie is done \n");
	
	close(sockfd);
	return 0;

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

/** Creates a stream message
	@param id The current stream id
	@param frame The current frame
	@param done Whether or not the stream is done
	@param data The message data
	@return A pointer to the newly created stream message, should be freed after use
*/

stream_msg_o* create_stream_msg(int id, int frame, int done, char* data) {
	stream_msg_o* msg = (stream_msg_o*) malloc(sizeof(stream_msg_o));
	msg->id = id;
	msg->frame = frame;
	msg->done = done;
	strncpy(msg->data, data, MAX_STREAM_DATA);
	
	return msg;
}

/** Returns the next available stream id
	@return The next stream id
*/

int get_next_stream_id() {
	return stream_id ++;
}


