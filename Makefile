.PHONY: examples clean

# Headers
API_INCLUDES = -I include/api

# Sources
API_SOURCES = $(wildcard src/api/*.c)

CC = gcc
CFLAGS = -g -DLOG_USE_COLOR=1 -Wall

main:
	$(CC) $(CFLAGS) -pthread  $(API_INCLUDES) ${API_SOURCES} src/leptonic.c -lzmq -o bin/leptonic

examples:
	@mkdir -p bin/examples/
	$(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/cci_do_ffc.c -o bin/examples/cci_do_ffc
	$(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/cci_set_agc.c -o bin/examples/cci_set_agc
	$(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/telemetry.c -o bin/examples/telemetry
	$(CC) $(CFLAGS) -pthread $(API_INCLUDES) ${API_SOURCES} examples/fb_video.c -o bin/examples/fb_video

clean:
	@rm -f *.o
	@rm bin/leptonic
