#ifndef VOSPI_H
#define VOSPI_H

#include <stdint.h>

// Flip byte order of a word
#define FLIP_WORD_BYTES(word) (word >> 8) | (word << 8)

// The size of a single VoSPI packet
#define VOSPI_PACKET_BYTES 164
#define VOSPI_PACKET_SYMBOLS 160

// The maximum number of packets per segment, sufficient to include telemetry
#define VOSPI_MAX_PACKETS_PER_SEGMENT 61
// The number of packets in segments with and without telemetry lines present
#define VOSPI_PACKETS_PER_SEGMENT_NORMAL 60
#define VOSPI_PACKETS_PER_SEGMENT_TELEMETRY 61

// The number of segments per frame
#define VOSPI_SEGMENTS_PER_FRAME 4

// The maximum number of resets allowed before giving up on synchronising
#define VOSPI_MAX_SYNC_RESETS 30
// The maximum number of invalid frames before giving up and assuming we've lost sync
// FFC duration is nominally 23 frames, so we should never exceed that
#define VOSPI_MAX_INVALID_FRAMES 25

// A single VoSPI packet
typedef struct {
	uint16_t id;
	uint16_t crc;
	uint8_t symbols[VOSPI_PACKET_SYMBOLS];
} vospi_packet_t;

// A single VoSPI segment
typedef struct {
	vospi_packet_t packets[VOSPI_MAX_PACKETS_PER_SEGMENT];
	int packet_count;
} vospi_segment_t;

// A single VoSPI frame
typedef struct {
	vospi_segment_t segments[VOSPI_SEGMENTS_PER_FRAME];
} vospi_frame_t;

int vospi_init(int fd, uint32_t speed);
int sync_and_transfer_frame(int fd, vospi_frame_t* frame);
int transfer_frame(int fd, vospi_frame_t* frame);

#endif /* VOSPI_H */
