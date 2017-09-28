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
#include "gpio.h"

static const char *device = "/dev/spidev0.0";
static uint8_t mode = SPI_MODE_3 | SPI_NO_CS;
static uint8_t bits = 8;
static uint32_t speed = 18000000;

// The chip-select pin
#define CS_BCM_PIN 25

// The size of a single VOSPI packet
#define VOSPI_PACKET_BYTES 164

// The number of packets per segment
#define VOSPI_PACKETS_PER_SEGMENT 60

// The number of segments per frame
#define VOSPI_SEGMENTS_PER_FRAME 4

// An array containing the bytes from a single packet
uint8_t lepton_packet[VOSPI_PACKET_BYTES];

typedef struct vospi_packet {
	uint8_t symbols[VOSPI_PACKET_BYTES];
} vospi_packet_t;

typedef struct vospi_segment {
	vospi_packet_t packets[VOSPI_PACKETS_PER_SEGMENT];
} vospi_segment_t;

/**
 * Transfer a single VoSPI segment.
 */
void transfer_segment(int fd, vospi_segment_t* segment)
{
	// Perform the spidev transfer
	int ret = read(fd, &segment->packets[0].symbols, VOSPI_PACKET_BYTES);
	if (ret < 1) {
		perror("SPI: failed to transfer packet");
		exit(-5);
	}

	while (segment->packets[0].symbols[0] & 0x0f == 0x0f) {
		// It was a discard packet, try receiving another packet into the same buf
    read(fd, &segment->packets[0].symbols, VOSPI_PACKET_BYTES);
	}

	// Read the remaining packets
	ret = read(fd, &segment->packets[1].symbols, VOSPI_PACKET_BYTES * (VOSPI_PACKETS_PER_SEGMENT - 1));
	if (ret < 1) {
		perror("SPI: failed to transfer the rest of the segment");
		exit(-5);
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

	// Set up the GPIO-based CS pin
	printf("Setting up GPIO-CS...\n");
	gpio_export(CS_BCM_PIN);
	gpio_direction(CS_BCM_PIN, GPIO_OUT);
	gpio_write(CS_BCM_PIN, GPIO_HIGH);


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

	// Select the chip
	gpio_write(CS_BCM_PIN, GPIO_LOW);
	usleep(100000);

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
					gpio_write(CS_BCM_PIN, GPIO_HIGH);
					usleep(185000);
					gpio_write(CS_BCM_PIN, GPIO_LOW);
					continue;
			}

			// Check we're looking at the first segment, if not, just keep reading until we get there
			ttt_bits = segments[0]->packets[20].symbols[0] >> 4;
			printf("TTT Bits: %d\t P20 Num: %d\n", ttt_bits, packet_20_num);
			if (ttt_bits == 1 && packet_20_num == 20) {
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
