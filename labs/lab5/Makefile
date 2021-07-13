all: threads

threads: threads.cc iodas.o
	g++ -Wall -L/lib/thread -o threads threads.cc iodas.o

iodas.o: iodas.c
	gcc -Wall -c iodas.c

clean:
	rm -f threads iodas.o
