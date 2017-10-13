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
 *
 * This example enables telemetry data, transfers a single frame then displays the received information.
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
	vospi_frame_t frame;
  for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
    frame.segments[seg].packet_count = VOSPI_PACKETS_PER_SEGMENT_TELEMETRY;
  }

	// Synchronise and transfer a single frame
	log_info("aquiring VoSPI synchronisation");
	if (0 == sync_and_transfer_frame(spi_fd, &frame)) {
		log_error("failed to obtain frame from device.");
    exit(-10);
	}
	log_info("VoSPI stream synchronised");

	// Parse the telemetry data
	telemetry_data_t data = parse_telemetry_packet(&(frame.segments[0].packets[0]));

  log_info("Telmetry data decoded:");
	log_info("Msec since boot: %02x", data.msec_since_boot);
	log_info("Msec since last FFC: %02x", data.msec_last_ffc);
	log_info("Frame mean: %02x", data.frame_mean);
	log_info("FPA Temp Kelvin100: %02x", data.fpa_temp_kelvin_100);
	log_info("FFC Desired: %02x", data.status_bits.ffc_desired);
	log_info("FFC State: %02x", data.status_bits.ffc_state);
	log_info("AGC State: %02x", data.status_bits.agc_state);
	log_info("Shutter locked?: %02x", data.status_bits.shutter_lockout);
	log_info("Overtemp shutdown imminent?: %02x", data.status_bits.overtemp_shutdown_imminent);

  // Disable telemetry again to leave the module in a usable state for other examples
  cci_set_telemetry_enable_state(i2c_fd, CCI_TELEMETRY_DISABLED);

  // Close files
	close(spi_fd);
  close(i2c_fd);

	return 0;
}
