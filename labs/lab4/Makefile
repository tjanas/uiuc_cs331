all: part1 part2
part1: io display
part2: threads

io: io.cc iodas.o
	g++ -Wall -o io io.cc iodas.o

display: display.cc iodas.o
	g++ -Wall -o display display.cc iodas.o

threads: threads.cc iodas.o
	g++ -Wall -L/lib/thread -o threads threads.cc iodas.o

iodas.o: iodas.c
	gcc -Wall -c iodas.c

clean:
	rm -f io display threads iodas.o
