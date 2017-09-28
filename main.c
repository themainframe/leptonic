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
#include "vospi.h"

static const char *device = "/dev/spidev0.0";
static uint8_t mode = SPI_MODE_3;
static uint8_t bits = 8;
static uint32_t speed = 18000000;

/**
 * Transfer a single VoSPI segment.
 */
void transfer_segment(int fd, vospi_segment_t* segment)
{
	// Perform the spidev transfer
	if (read(fd, &segment->packets[0].symbols, VOSPI_PACKET_BYTES) < 1) {
		perror("SPI: failed to transfer packet");
		exit(-5);
	}

	while (segment->packets[0].symbols[0] & 0x0f == 0x0f) {
		// It was a discard packet, try receiving another packet into the same buf
    read(fd, &segment->packets[0].symbols, VOSPI_PACKET_BYTES);
	}

	// Read the remaining packets
	if (read(fd, &segment->packets[1].symbols, VOSPI_PACKET_BYTES * (VOSPI_PACKETS_PER_SEGMENT - 1)) < 1) {
		perror("SPI: failed to transfer the rest of the segment");
		exit(-5);
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

	// Open the spidev device
	printf("Opening device...\n");
	fd = open(device, O_RDWR);
	if (fd < 0) {
		perror("SPI: failed to open device - check permissions & spidev enabled");
		exit(-1);
	}

	// Set the various SPI parameters
	printf("Setting device mode...\n");
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		perror("SPI: failed to set mode");
		exit(-2);
	}
	printf("Setting bits/word...\n");
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		perror("SPI: failed to set the bits per word option");
		exit(-3);
	}
	printf("Setting speed...\n");
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		perror("SPI: failed to set the max speed option");
		exit(-4);
	}

	// Allocate space to receive the segments
	printf("Allocating space for segments...\n");
	vospi_segment_t* segments[VOSPI_SEGMENTS_PER_FRAME];
	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		segments[seg] = malloc(sizeof(vospi_segment_t));
	}

	// Keep streaming segments until we receive a valid, first segment to sync
	printf("Synchronising with first segment...\n");
	uint8_t ttt_bits, packet_20_num;

	while (1) {

			// Stream a first segment
			printf("Receiving...\n");
			transfer_segment(fd, segments[0]);

			// If the packet number isn't even correct, we'll reset the bus to sync
			packet_20_num = segments[0]->packets[20].symbols[1];
			if (packet_20_num != 20) {
					// Deselect the chip, wait 200ms with CS deasserted
					printf("Waiting to reset...\n");
					usleep(185000);
					continue;
			}

			// Check we're looking at the first segment, if not, just keep reading until we get there
			ttt_bits = segments[0]->packets[20].symbols[0] >> 4;
			printf("TTT Bits: %d\t P20 Num: %d\n", ttt_bits, packet_20_num);
			if (ttt_bits == 1) {
				break;
			}
	}

	// Receive the remaining segments
	for (int seg = 1; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		transfer_segment(fd, segments[seg]);
	}

	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		for (int i = 0; i < VOSPI_PACKET_BYTES; i ++) {
			printf("%02x ", segments[seg]->packets[20].symbols[i]);
		}
		printf("\n\n");
	}

	close(fd);
	return ret;
}
