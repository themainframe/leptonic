#include "log/log.h"
#include "vospi/vospi.h"
#include "main.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <limits.h>

static uint8_t mode = SPI_MODE_3;
static uint8_t bits = 8;
static uint32_t speed = 18000000;

/**
 * Main entry point for Leptonic.
 */
int main(int argc, char *argv[])
{
	log_set_level(LOG_INFO);
	int ret = 0;
	int fd;

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

	// Set the various SPI parameters
	log_debug("setting SPI device mode...");
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		log_fatal("SPI: failed to set mode");
		exit(-2);
	}
	log_debug("setting SPI bits/word...");
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		log_fatal("SPI: failed to set the bits per word option");
		exit(-3);
	}
	log_debug("setting SPI max clock speed...");
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		log_fatal("SPI: failed to set the max speed option");
		exit(-4);
	}

	// Allocate space to receive the segments
	log_debug("allocating space for segments...");
	vospi_segment_t* segments[VOSPI_SEGMENTS_PER_FRAME];
	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		segments[seg] = malloc(sizeof(vospi_segment_t));
	}

	// Synchronise and transfer a single frame
	log_info("aquiring VoSPI synchronisation");
	int resets;
	if (!(resets = sync_and_transfer_frame(fd, segments))) {
		log_error("failed to obtain frame from device.");
    exit(-10);
	}
	log_info("VoSPI stream synchronised (%d resets required)", resets);

	do {

			for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
				for (int i = 0; i < VOSPI_PACKET_BYTES; i ++) {
					printf("%02x ", segments[seg]->packets[20].symbols[i]);
				}
				printf("\n\n");
			}

			transfer_frame(fd, segments);

	} while (1);


	close(fd);
	return ret;
}
