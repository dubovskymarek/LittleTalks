
CC = gcc

#build library and all examples
LittleTalksLib:
	$(CC) -c lib/LTPlatformAdapter.c
	$(CC) -c lib/LTDevice.c LTPlatformAdapter.o
	$(CC) -c lib/LTTopic.c LTPlatformAdapter.o
	$(CC) -c lib/LittleTalksPrivate.c LTPlatformAdapter.o LTDevice.o LTTopic.o
	$(CC) -c lib/LittleTalks.c LTPlatformAdapter.o LTDevice.o LTTopic.o LittleTalksPrivate.o
	ar rcs build/libLittleTalks.a LTPlatformAdapter.o LTDevice.o LTTopic.o LittleTalksPrivate.o LittleTalks.o
	#make examples
	make rootMake -f examples/example1/makefile

#clean library and all examples
clean:
	rm -f LTPlatformAdapter.o
	rm -f LTDevice.o
	rm -f LTTopic.o
	rm -f LittleTalksPrivate.o
	rm -f LittleTalks.o
	rm -f build/*
	#clean examples
	make clean -f examples/example1/makefile