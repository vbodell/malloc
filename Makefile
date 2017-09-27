CC = gcc

CFLAGS = -Wall -g -fpic

malloc: malloc.o
	ar -r libmalloc.a malloc.o

try: tryme.o malloc
	$(CC) $(CFLAGS) -o tryme tryme.o libmalloc.a

main: main.c malloc
	$(CC) $(CFLAGS) -o main main.c libmalloc.a

tryme.o: tryme.c
	$(CC) $(CFLAGS) -c tryme.c

malloc.o: malloc.c malloc.h
	$(CC) $(CFLAGS) -c malloc.c

intel-all: lib64/libmalloc.so lib/libmalloc.so
	$(CC) $(CFLAGS) -m32 -shared -o $@ malloc32.o

lib/libmalloc.so: lib malloc32.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o

lib64/libmalloc.so: lib64 malloc64.o
	$(CC) $(CFLAGS) -shared -o $@ malloc64.o


lib:
	mkdir lib

lib64:
	mkdir lib64

malloc32.o: malloc.c
	$(CC) $(CFLAGS) -m32 -c -o malloc32.o malloc.c

malloc64.o: malloc.c
	$(CC) $(CFLAGS) -m64 -c -o malloc64.o malloc.c

clean:
	rm *.o *.txt tryme main
