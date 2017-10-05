#include "log.h"
#include "vospi.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

/**
 * Main entry point for example.
 */
int main(int argc, char *argv[])
{
	log_set_level(LOG_INFO);
	int fd;

	// Remind the user about using this example after the telemetry ones
	log_info("Note that this example assumes the Lepton is in the default startup state.");
	log_info("If you've already run the telemetry examples, it likely won't work without a restart.");

  // Check we have enough arguments to work
  if (argc < 2) {
    log_error("Can't start - SPI device file path must be specified.");
    exit(-1);
  }

	// Open the spidev device
	log_info("opening SPI device...");
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		log_fatal("SPI: failed to open device - check permissions & spidev enabled");
		exit(-1);
	}

	// Initialise the VoSPI interface
	if (vospi_init(fd, 18000000) == -1) {
			log_fatal("SPI: failed condition SPI device for VoSPI use.");
			exit(-1);
	}

	// Allocate space to receive the segments
	log_debug("allocating space for segments...");
	vospi_segment_t* segments[VOSPI_SEGMENTS_PER_FRAME];
	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		segments[seg] = malloc(sizeof(vospi_segment_t));
	}

	// Synchronise and transfer a single frame
	log_info("aquiring VoSPI synchronisation");
	if (0 == sync_and_transfer_frame(fd, segments, TELEMETRY_DISABLED)) {
		log_error("failed to obtain frame from device.");
    exit(-10);
	}
	log_info("VoSPI stream synchronised");

	do {
			for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {

				printf("%02x --> ", segments[seg]->packets[20].id >> 12);
				for (int i = 0; i < VOSPI_PACKET_BYTES; i ++) {
					printf("%02x ", segments[seg]->packets[20].symbols[i]);
				}
				printf("\n\n");
			}
			transfer_frame(fd, segments, TELEMETRY_DISABLED);
	} while (1);

	close(fd);
	return 0;
}
