.PHONY: examples clean

# Headers
API_INCLUDES = -Iinclude/api

# Sources
API_SOURCES = $(wildcard src/api/*.c)

# GTK Includes
GTK_INCLUDES = `pkg-config --cflags --libs gtk+-3.0`

CC = gcc
CFLAGS = -g -DLOG_USE_COLOR=1 -Wall

main:
	$(CC) $(CFLAGS) -pthread -lzmq $(API_INCLUDES) ${API_SOURCES} src/leptonic.c -o bin/leptonic

examples:
	@mkdir -p bin/examples/
	# $(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/cci_do_ffc.c -o bin/examples/cci_do_ffc
	# $(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/cci_set_agc.c -o bin/examples/cci_set_agc
	# $(CC) $(CFLAGS) $(API_INCLUDES) ${API_SOURCES} examples/telemetry.c -o bin/examples/telemetry
	# $(CC) $(CFLAGS) $(API_INCLUDES) ${GTK_INCLUDES} ${API_SOURCES} examples/gtk_video.c -o bin/examples/gtk_video
	$(CC) $(CFLAGS) -pthread  $(API_INCLUDES) ${API_SOURCES} examples/fb_video.c -o bin/examples/fb_video

clean:
	@rm -f *.o
	@rm leptonic
