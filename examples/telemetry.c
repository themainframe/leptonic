#include "log.h"
#include "vospi.h"
#include "cci.h"
#include "telemetry.h"
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
	int spi_fd, i2c_fd;

  // Check we have enough arguments to work
  if (argc < 3) {
    log_error("Can't start - SPI and CCI I2C device file paths must be specified.");
    exit(-1);
  }

	// Open the spidev device
	log_info("opening SPI device...");
	spi_fd = open(argv[1], O_RDWR);
	if (spi_fd < 0) {
		log_fatal("SPI: failed to open device - check permissions & spidev enabled");
		exit(-1);
	}

	// Open the CCI device
	log_info("opening CCI I2C device...");
	i2c_fd = open(argv[2], O_RDWR);
	if (i2c_fd < 0) {
		log_fatal("I2C: failed to open device - check permissions & I2C enabled");
		exit(-1);
	}

	// Initialise the VoSPI interface
	if (vospi_init(spi_fd, 18000000) == -1) {
			log_fatal("SPI: failed condition SPI device for VoSPI use.");
			exit(-1);
	}

	// Initialise the I2C interface
	if (cci_init(i2c_fd) == -1) {
			log_fatal("I2C: failed condition I2C device for CCI use.");
			exit(-1);
	}

  // Enable telemetry in the footer of segments
	cci_init(i2c_fd);
  cci_set_telemetry_enable_state(i2c_fd, CCI_TELEMETRY_ENABLED);
  cci_set_telemetry_location(i2c_fd, CCI_TELEMETRY_LOCATION_HEADER);
	log_info("CCI uptime: %d", cci_get_uptime(i2c_fd));
	log_info("CCI telemetry enable state: %d", cci_get_telemetry_enable_state(i2c_fd));
	log_info("CCI telemetry location: %d", cci_get_telemetry_location(i2c_fd));

	// Allocate space to receive the segments
	log_debug("allocating space for segments...");
	vospi_segment_t* segments[VOSPI_SEGMENTS_PER_FRAME];
	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		segments[seg] = malloc(sizeof(vospi_segment_t));
	}

	// Synchronise and transfer a single frame
	log_info("aquiring VoSPI synchronisation");
	if (0 == sync_and_transfer_frame(spi_fd, segments, TELEMETRY_ENABLED)) {
		log_error("failed to obtain frame from device.");
    exit(-10);
	}
	log_info("VoSPI stream synchronised");

	do {

			for (int i = 0; i < VOSPI_PACKET_BYTES; i ++) {
				printf("%02x ", segments[0]->packets[0].symbols[i]);
			}
			printf("\n\n");
			transfer_frame(spi_fd, segments, TELEMETRY_ENABLED);
	} while (1);

	close(spi_fd);
	return 0;
}
