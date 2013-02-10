CC=gcc 
CFLAGS=-std=c99

all: src/goatpress

scr/goatpress: src/goatpress.o

clean:
	rm src/goatpress.o src/goatpress
