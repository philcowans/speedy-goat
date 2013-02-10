CC=gcc 
CFLAGS=-std=c99 -g

all: src/goatpress

src/goatpress: src/goatpress.o

clean:
	rm src/goatpress.o src/goatpress
