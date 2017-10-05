#ifndef TELEMETRY_H
#define TELEMETRY_H
#include <stdint.h>

/* Telemetry Data Content (see pg. 23 of the Lepton3 LWIR Datasheet) */
typedef struct {
  uint16_t revision;
  uint32_t msec_since_boot;
  uint32_t status_bits;
  unsigned char reserved_1[16];
  uint64_t software_rev;
  unsigned char reserved_2[6];
  uint32_t frame_count;
  uint16_t frame_mean;
  uint16_t fpa_temp_count;
  uint16_t fpa_temp_kelvin_100;
  unsigned char reserved_3[8];
  uint16_t fpa_temp_last_ffc_kelvin_100;
  uint32_t msec_last_ffc;
  unsigned char reserved_4[4];
  uint64_t agc_roi;
  uint16_t agc_clip_limit_high;
  uint16_t agc_clip_limit_low;
  unsigned char reserved_5[64];
  uint32_t video_output_format;
  unsigned char reserved_6[172];
} telemetry_data_t;



#endif
