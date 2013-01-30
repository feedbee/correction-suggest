CFLAGS=-std=c99

all: dict-build suggest2

# dict-build
dict-build: dict-build.c
	gcc $(CFLAGS) -o dict-build dict-build.c

#suggest2
suggest2: suggest2.o levenstein.o
	gcc $(CFLAGS) -o suggest2 suggest2.o levenstein.o -lrt

suggest2.o: suggest2.c suggest2.h levenstein.h
	gcc $(CFLAGS) -Ofast -D_POSIX_C_SOURCE=200112L -c suggest2.c

levenstein.o: levenstein.c levenstein.h
	gcc $(CFLAGS) -ffast-math -ffloat-store -funsafe-math-optimizations -Ofast -c levenstein.c

# clean
clean:
	rm -f *.o dict-build suggest2
