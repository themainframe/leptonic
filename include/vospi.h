#ifndef VOSPI_H
#define VOSPI_H

#include <stdint.h>

// Flip byte order of a word
#define FLIP_WORD_BYTES(word) (word >> 8) | (word << 8)

// The size of a single VoSPI packet
#define VOSPI_PACKET_BYTES 164

// The (normal) number of packets per segment
#define VOSPI_PACKETS_PER_SEGMENT 60

// The number of segments per frame
#define VOSPI_SEGMENTS_PER_FRAME 4

// The maximum number of resets allowed before giving up on synchronising
#define VOSPI_MAX_SYNC_RESETS 30

// Telemetry Mode
typedef enum {
	TELEMETRY_DISABLED,
	TELEMETRY_ENABLED
} vospi_telemetry_mode_t;

// A single VoSPI packet
typedef struct {
	uint16_t id;
	uint16_t crc;
	uint8_t symbols[VOSPI_PACKET_BYTES - 4];
} vospi_packet_t;

// A single VoSPI segment
typedef struct {
	vospi_packet_t packets[VOSPI_PACKETS_PER_SEGMENT];
} vospi_segment_t;

int vospi_init(int fd, uint32_t speed);
int sync_and_transfer_frame(int fd, vospi_segment_t** segments, vospi_telemetry_mode_t telemetry_mode);
int transfer_frame(int fd, vospi_segment_t** segments, vospi_telemetry_mode_t telemetry_mode);

#endif /* VOSPI_H */
