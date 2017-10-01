#include <stdint.h>

#ifndef VOSPI_H
#define VOSPI_H

// The size of a single VoSPI packet
#define VOSPI_PACKET_BYTES 164

// The number of packets per segment
#define VOSPI_PACKETS_PER_SEGMENT 60

// The number of segments per frame
#define VOSPI_SEGMENTS_PER_FRAME 4

// A single VoSPI packet
typedef struct {
	uint8_t symbols[VOSPI_PACKET_BYTES];
} vospi_packet_t;

// A single VoSPI segment
typedef struct {
	vospi_packet_t packets[VOSPI_PACKETS_PER_SEGMENT];
} vospi_segment_t;

int sync_and_transfer_frame(int fd, vospi_segment_t** segments);
int transfer_frame(int fd, vospi_segment_t** segments);

#endif /* VOSPI_H */
