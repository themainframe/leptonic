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

static const char *device = "/dev/spidev0.1";
static uint8_t mode = SPI_MODE_3;
static uint8_t bits = 8;
static uint32_t speed = 18000000;

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
	// Create the spidev message
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long){0, },
		.rx_buf = (unsigned long)(segment->packets[0].symbols),
		.len = VOSPI_PACKET_BYTES,
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
		.cs_change = 1
	};

	// Perform the spidev transfer
	int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		perror("SPI: failed to transfer packet");
		exit(-5);
	}

	while (segment->packets[0].symbols[0] & 0xf0 == 0xf0) {
		// It was a discard packet, try receiving another packet into the same buf
		ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	}

	// Read the remaining packets
	tr.rx_buf = (unsigned long)(segment->packets[1]);
	tr.len = VOSPI_PACKET_BYTES * (VOSPI_PACKETS_PER_SEGMENT - 1);
	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		perror("SPI: failed to transfer the rest of the segment");
		exit(-5);
	}
}

int main(int argc, char *argv[])
{
	int ret = 0;
	int fd;

	// Open the spidev device
	fd = open(device, O_RDWR);
	if (fd < 0) {
		perror("SPI: failed to open device - check permissions & spidev enabled");
		exit(-1);
	}

	// Set the various SPI parameters
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		perror("SPI: failed to set mode");
		exit(-2);
	}
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		perror("SPI: failed to set the bits per word option");
		exit(-3);
	}
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		perror("SPI: failed to set the max speed option");
		exit(-4);
	}

	// Allocate space to receive the segments
	vospi_segment_t* segments[VOSPI_SEGMENTS_PER_FRAME];
	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		segments[seg] = malloc(sizeof(vospi_segment_t));
	}

	// Stream our first segment
	transfer_segment(fd, segments[0]);

	// Keep streaming segments until we receive a valid, first segment
	printf("TTT: %d\n", (segments[0]->packets[20].symbols[0] >> 4) & 0x0f);
	while (((segments[0]->packets[20].symbols[0] >> 4) & 0x0f) != 1) {
		printf("TTT: %d\n", (segments[0]->packets[20].symbols[0] >> 4) & 0x0f);
		usleep(200000);
		transfer_segment(fd, segments[0]);
	}

	// Receive the remaining segments
	for (int seg = 1; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		transfer_segment(fd, segments[seg]);
	}

	for (int seg = 0; seg < VOSPI_SEGMENTS_PER_FRAME; seg ++) {
		printf ("Segment %d:\n", seg);
		printf("TTT: %d\n", (segments[seg]->packets[20].symbols[0] >> 4) & 0x0f);
		for (int i = 0; i < VOSPI_PACKET_BYTES; i ++) {
			printf("%02x ", segments[seg]->packets[20].symbols[i]);
		}
		printf("\n\n");
	}

	close(fd);
	return ret;
}
