LAB = lab1

$(LAB): iodas.o $(LAB).cc
	g++ -o $(LAB) $(LAB).cc iodas.o

iodas.o: iodas.c
	gcc -c iodas.c

clean:
	rm -f $(LAB) iodas.o
