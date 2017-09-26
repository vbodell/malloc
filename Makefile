CC = gcc

CFLAGS = -Wall -g -fpic

malloc: malloc.o
	$(CC) -Wall -g -o malloc malloc.c

try: tryme.o
	$(CC) $(CFLAGS) -L lib64/ -o tryme tryme.o -lmalloc

main: main.c mylib
	$(CC) $(CFLAGS) -o main main.c libmalloc.a

mylib: malloc.o
	ar -r libmalloc.a malloc.o

tryme.o: tryme.c
	$(CC) $(CFLAGS) -c tryme.c

malloc.o: malloc.c
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
	rm *.o


