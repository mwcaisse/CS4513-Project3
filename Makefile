all: nutella player

player: player.o util.o
	gcc $^ -o player
	
nutella: nutella.o msock.o util.o
	gcc $^ -o nutella -pthread -lreadline
	
player.o: player.c player.h
	gcc -c player.c

nutella.o: nutella.c nutella.h
	gcc -c nutella.c
	
util.o: util.c util.h
	gcc -c util.c
	
msock.o: msock.c msock.h
	gcc -c msock.c
	
clean:
	rm -f *.o nutella