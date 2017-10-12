.PHONY: examples clean

# Headers
INCLUDES = -Iinclude/

# Sources
SOURCES = $(wildcard src/*.c)

CC = gcc
CFLAGS = -g -DLOG_USE_COLOR=1 -pthread -lzmq

examples:
	@mkdir -p bin/examples/
	# $(CC) $(CFLAGS) $(INCLUDES) ${SOURCES} examples/sync_and_get_frames.c -o bin/examples/sync_and_get_frames
	$(CC) $(CFLAGS) $(INCLUDES) ${SOURCES} examples/cci_do_ffc.c -o bin/examples/cci_do_ffc
	# $(CC) $(CFLAGS) $(INCLUDES) ${SOURCES} examples/telemetry.c -o bin/examples/telemetry
	$(CC) $(CFLAGS) $(INCLUDES) ${SOURCES} examples/zmq_video_server.c -o bin/examples/zmq_video_server
	# $(CC) $(CFLAGS) $(INCLUDES) ${SOURCES} examples/zmq_cci_server.c -o bin/examples/zmq_cci_server

clean:
	@rm -f *.o
	@rm leptonic
