CC = gcc

CFLAGS = -Wall -g -fpic

malloc: malloc.o
	$(CC) -Wall -g -o malloc malloc.c

try: tryme.o
	$(CC) $(CFLAGS) -L lib64/ -o tryme tryme.o -lmalloc

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



make intel-all
gcc -Wall -g -fpic -m32 -c -o malloc32.o malloc.c
In file included from /usr/include/features.h:399:0,
                 from /usr/include/stdio.h:27,
                 from malloc.c:12:
/usr/include/gnu/stubs.h:7:27: fatal error: gnu/stubs-32.h: No such file or directory
 # include <gnu/stubs-32.h>
                           ^
compilation terminated.
make: *** [malloc32.o] Error 1

Build failed.

