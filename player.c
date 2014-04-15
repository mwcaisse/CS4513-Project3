
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "player.h"
#include "util.h"


#define FPS 1



int main (int argc, char* argv[]) {

	//the movie file to open
	char* file_name = "movies/starwars.txt";
	
	
	FILE* file = fopen(file_name, "r");
	
	if (!file) {
		perror("Couldnt open file");
		return 1;
	}
	
	char* line = NULL;
	size_t len = 0;
	
	int read = 0;
	
	int sleep_time = (1000 / FPS) * 1000;
	
	while ( (read = getline(&line, &len, file)) != -1) {
		if (strncmp(line, "end", 3) == 0) {
			//we have reached  the end of a frame
			usleep(sleep_time);
			clear_screen();
		}
		else {
			printf("%s\n", line);
		}
	}
	
	printf("Movie is over bitch \n");

	return 0;
}


