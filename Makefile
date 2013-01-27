CFLAGS=-std=c99 -g

all: dict-build suggest

# dict-build
dict-build: dict-build.c
	gcc $(CFLAGS) -o dict-build dict-build.c

#suggest
suggest: suggest.o levenstein.o
	gcc $(CFLAGS) -o suggest suggest.o levenstein.o -lrt

suggest.o: suggest.c suggest.h levenstein.h
	gcc $(CFLAGS) -D_POSIX_C_SOURCE=200112L -c suggest.c

levenshtein.o: levenstein.c levenstein.h
	gcc $(CFLAGS) -c levenstein.c

# clean
clean:
	rm -f *.o dict-build suggest