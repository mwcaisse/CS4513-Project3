all: nutella
	
nutella: nutella.o msock.o
	gcc $^ -o nutella -pthread -lreadline
	
nutella.o: nutella.c nutella.h
	gcc -c nutella.c
	
msock.o: msock.c msock.h
	gcc -c msock.c
	
clean:
	rm -f *.o nutella