#ifndef TELEMETRY_H
#define TELEMETRY_H
#include "vospi.h"
#include <stdint.h>

#define LEPTON_WORD(buf, i) buf[i] << 8 | buf[i + 1]
#define LEPTON_DWORD(buf, i) LEPTON_WORD(buf, i + 2) << 16 | LEPTON_WORD(buf,i)
#define LEPTON_QWORD(buf, i) LEPTON_DWORD(buf, i + 4) << 32 | LEPTON_DWORD(buf,i)

/** Telemetry Data Status Bits field **/
typedef struct {
  uint8_t ffc_desired;
  uint8_t ffc_state;
  uint8_t agc_state;
  uint8_t reserved_2;
  uint8_t shutter_lockout;
  uint8_t reserved_3;
  uint8_t overtemp_shutdown_imminent;
  uint16_t reserved_4;
} telemetry_data_status_bits_t;

/* Telemetry Data Content (see pg. 23 of the Lepton3 LWIR Datasheet) */
/* Only covers Telemetry Row A, as there's nothing interesting in B... */
typedef struct {
  uint16_t revision;
  uint32_t msec_since_boot;
  telemetry_data_status_bits_t status_bits;
  uint64_t software_rev;
  uint32_t frame_count;
  uint16_t frame_mean;
  uint16_t fpa_temp_count;
  uint16_t fpa_temp_kelvin_100;
  uint16_t fpa_temp_last_ffc_kelvin_100;
  uint32_t msec_last_ffc;
  uint16_t agc_roi_top;
  uint16_t agc_roi_left;
  uint16_t agc_roi_bottom;
  uint16_t agc_roi_right;
  uint16_t agc_clip_limit_high;
  uint16_t agc_clip_limit_low;
  uint32_t video_output_format;
} telemetry_data_t;

telemetry_data_t parse_telemetry_packet(vospi_packet_t* packet);

#endif
