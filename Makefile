.PHONY: all clean

# Headers
INCLUDES = -Iinclude/ \
	-Iinclude/log \
	-Iinclude/vospi

# Sources
SOURCES = $(wildcard src/*.c) \
	$(wildcard src/cci/*.c) \
	$(wildcard src/log/*.c)\
	$(wildcard src/vospi/*.c)

CC = gcc
CFLAGS = -g -DLOG_USE_COLOR=1

all: ${SOURCES}
	@mkdir -p bin/
	$(CC) $(CFLAGS) $(INCLUDES) ${SOURCES} -o leptonic

clean:
	@rm -f *.o
	@rm leptonic
