.PHONY: examples clean

# Headers
API_INCLUDES = -Iinclude/api

# Sources
API_SOURCES = $(wildcard src/api/*.c)

CC = gcc
CFLAGS = -g -DLOG_USE_COLOR=1 -Wall

main:
	$(CC) $(CFLAGS) -pthread -lzmq $(API_INCLUDES) ${API_SOURCES} src/leptonic.c -o bin/leptonic

examples:
	@mkdir -p bin/examples/
	$(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/cci_do_ffc.c -o bin/examples/cci_do_ffc
	$(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/telemetry.c -o bin/examples/telemetry

clean:
	@rm -f *.o
	@rm leptonic
