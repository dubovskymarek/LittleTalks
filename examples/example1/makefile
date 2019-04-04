
CC = gcc
INC=-I../../lib/
LIB=-L../../build
 
LittleTalksLib:
	$(CC) -c $(INC) main.c
	$(CC) -o ../../build/example1 main.o ../../build/libLittleTalks.a
 
rootMake:
	$(CC) -c -Ilib/ examples/example1/main.c
	$(CC) -o build/example1 main.o build/libLittleTalks.a
	
clean:
	rm -f main.o