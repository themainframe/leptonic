#include "log/log.h"
#include "vospi/vospi.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <limits.h>

/**
 * Transfer a single VoSPI segment.
 */
void transfer_segment(int fd, vospi_segment_t* segment)
{
	// Perform the spidev transfer
	if (read(fd, &segment->packets[0].symbols, VOSPI_PACKET_BYTES) < 1) {
		log_fatal("SPI: failed to transfer packet");
		exit(-5);
	}

	while (segment->packets[0].symbols[0] & 0x0f == 0x0f) {
		// It was a discard packet, try receiving another packet into the same buf
    read(fd, &segment->packets[0].symbols, VOSPI_PACKET_BYTES);
	}

	// Read the remaining packets
	if (read(fd, &segment->packets[1].symbols, VOSPI_PACKET_BYTES * (VOSPI_PACKETS_PER_SEGMENT - 1)) < 1) {
		log_fatal(
      "SPI: failed to transfer the rest of the segment - "
      "Check to ensure that the bufsiz module parameter for spidev is set to > %d bytes",
      VOSPI_PACKET_BYTES * VOSPI_PACKETS_PER_SEGMENT
    );
		exit(-5);
	}
}

/**
 * Synchroise the VoSPI stream and transfer a single frame.
 */
int sync_and_transfer_frame(int fd, vospi_segment_t** segments)
{
  // Keep streaming segments until we receive a valid, first segment to sync
	log_debug("synchronising with first segment");
	uint8_t ttt_bits, packet_20_num, resets = 0;

  while (1) {

			// Stream a first segment
			log_debug("receiving first segment...");
			transfer_segment(fd, segments[0]);

			// If the packet number isn't even correct, we'll reset the bus to sync
			packet_20_num = segments[0]->packets[20].symbols[1];
			if (packet_20_num != 20) {
					// Deselect the chip, wait 200ms with CS deasserted
					log_debug("deasserting CS & waiting to reset...");
					usleep(185000);
					resets ++;
					continue;
			}

			// Check we're looking at the first segment, if not, just keep reading until we get there
			ttt_bits = segments[0]->packets[20].symbols[0] >> 4;
			log_debug("TTT bits were: %dm P20 Num: %d", ttt_bits, packet_20_num);
			if (ttt_bits == 1) {
				break;
			}
	}

	// Receive the remaining segments
	for (int seg = 1; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		transfer_segment(fd, segments[seg]);
	}

  return resets;
}

/**
 * Transfer a frame.
 * Assumes that we're already synchronised with the VoSPI stream.
 */
int transfer_frame(int fd, vospi_segment_t** segments)
{
	uint8_t ttt_bits;

	// Receive all segments
	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		transfer_segment(fd, segments[seg]);

		ttt_bits = segments[0]->packets[20].symbols[0] >> 4;
		log_debug("TTT bits were: %d", ttt_bits);
		if (ttt_bits == 0) {
			seg --;
			continue;
		}
	}

  return 1;
}
