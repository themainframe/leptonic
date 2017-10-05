#include "log.h"
#include "vospi.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>

/**
 * Initialise the VoSPI interface.
 */
int vospi_init(int fd, uint32_t speed)
{
	// Set the various SPI parameters
	log_debug("setting SPI device mode...");
	uint16_t mode = SPI_MODE_3;
	if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1) {
		log_fatal("SPI: failed to set mode");
		return -1;
	}

	log_debug("setting SPI bits/word...");
	uint8_t bits = 8;
	if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
		log_fatal("SPI: failed to set the bits per word option");
		return -1;
	}

	log_debug("setting SPI max clock speed...");
	if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
		log_fatal("SPI: failed to set the max speed option");
		return -1;
	}

	return 1;
}

/**
 * Transfer a single VoSPI segment.
 * Returns the number of successfully-transferred segments (0 or 1).
 */
int transfer_segment(int fd, vospi_segment_t* segment, vospi_telemetry_mode_t telemetry_mode)
{
	// Perform the spidev transfer
	if (read(fd, &segment->packets[0], VOSPI_PACKET_BYTES) < 1) {
		log_fatal("SPI: failed to transfer packet");
		return 0;
	}

	// Flip the byte order of the ID & CRC
	segment->packets[0].id = FLIP_WORD_BYTES(segment->packets[0].id);
	segment->packets[0].crc = FLIP_WORD_BYTES(segment->packets[0].crc);

	while (segment->packets[0].id & 0x0f00 == 0x0f00) {
		// It was a discard packet, try receiving another packet into the same buf
    read(fd, &segment->packets[0], VOSPI_PACKET_BYTES);
	}

	// Read the remaining packets
	if (read(fd, &segment->packets[1], VOSPI_PACKET_BYTES * (VOSPI_PACKETS_PER_SEGMENT + telemetry_mode - 1)) < 1) {
		log_fatal(
      "SPI: failed to transfer the rest of the segment - "
      "Check to ensure that the bufsiz module parameter for spidev is set to > %d bytes",
      VOSPI_PACKET_BYTES * VOSPI_PACKETS_PER_SEGMENT
    );
		return 0;
	}

	// Flip the byte order for the rest of the packet IDs
	for (int i = 1; i < VOSPI_PACKETS_PER_SEGMENT; i ++) {
		segment->packets[i].id = FLIP_WORD_BYTES(segment->packets[i].id);
		segment->packets[i].crc = FLIP_WORD_BYTES(segment->packets[i].crc);
	}

	return 1;
}

/**
 * Synchroise the VoSPI stream and transfer a single frame.
 * Returns the number of successfully-transferred frames (0 or 1).
 */
int sync_and_transfer_frame(int fd, vospi_segment_t** segments, vospi_telemetry_mode_t telemetry_mode)
{
  // Keep streaming segments until we receive a valid, first segment to sync
	log_debug("synchronising with first segment");
	uint16_t packet_20_num;
	uint8_t ttt_bits, resets = 0;

  while (1) {

			// Stream a first segment
			log_debug("receiving first segment...");
			if (!transfer_segment(fd, segments[0], telemetry_mode)) {
				log_error("failed to receive the first segment");
				return 0;
			}

			// If the packet number isn't even correct, we'll reset the bus to sync
			packet_20_num = segments[0]->packets[20].id & 0xff;
			if (packet_20_num != 20) {
					// Deselect the chip, wait 200ms with CS deasserted
					log_info("packet 20 ID was %d - deasserting CS & waiting to reset...", packet_20_num);
					usleep(185000);

					if (++resets >= VOSPI_MAX_SYNC_RESETS) {
							log_error("too many resets while synchronising (%d)", resets);
							return 0;
					}

					continue;
			}

			// Check we're looking at the first segment, if not, just keep reading until we get there
			ttt_bits = segments[0]->packets[20].id >> 12;
			log_debug("TTT bits were: %dm P20 Num: %d", ttt_bits, packet_20_num);
			if (ttt_bits == 1) {
				break;
			}
	}

	// Receive the remaining segments
	for (int seg = 1; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		transfer_segment(fd, segments[seg], telemetry_mode);
	}

  return 1;
}

/**
 * Transfer a frame.
 * Assumes that we're already synchronised with the VoSPI stream.
 */
int transfer_frame(int fd, vospi_segment_t** segments, vospi_telemetry_mode_t telemetry_mode)
{
	uint8_t ttt_bits;

	// Receive all segments
	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		transfer_segment(fd, segments[seg], telemetry_mode);

		ttt_bits = segments[0]->packets[20].id >> 12;
		log_debug("TTT bits were: %d", ttt_bits);
		if (ttt_bits == 0) {
			seg --;
			continue;
		}
	}

  return 1;
}
