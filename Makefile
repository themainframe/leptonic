.PHONY: all clean

# Headers
INCLUDES = -Iinclude/

# Sources
SOURCES = $(wildcard *.c)
CC = gcc
CFLAGS = -g

all: ${SOURCES}
	@mkdir -p bin/
	$(CC) $(CFLAGS) $(INCLUDES) ${SOURCES} -o leptonic

clean:
	@rm -f *.o
	@rm leptonic
