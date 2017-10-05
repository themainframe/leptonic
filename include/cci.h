#ifndef CCI_H
#define CCI_H

#include <stdint.h>

/* CCI constants */
#define CCI_WORD_LENGTH 0x02
#define CCI_ADDRESS 0x2A

/* CCI register locations */
#define CCI_REG_STATUS 0x0002
#define CCI_REG_COMMAND 0x0004
#define CCI_REG_DATA_LENGTH 0x0006
#define CCI_REG_DATA_0 0x0008

/* Commands */
#define CCI_CMD_SYS_RUN_FFC 0x0242
#define CCI_CMD_SYS_GET_UPTIME 0x020C
#define CCI_CMD_SYS_GET_TELEMETRY_ENABLE_STATE 0x0218
#define CCI_CMD_SYS_SET_TELEMETRY_ENABLE_STATE 0x0219
#define CCI_CMD_SYS_GET_TELEMETRY_LOCATION 0x021C
#define CCI_CMD_SYS_SET_TELEMETRY_LOCATION 0x021D

/* Telemetry Modes for use with CCI_CMD_SYS_SET_TELEMETRY_* */
typedef enum {
  CCI_TELEMETRY_DISABLED,
  CCI_TELEMETRY_ENABLED,
} cci_telemetry_enable_state_t;

typedef enum {
  CCI_TELEMETRY_LOCATION_HEADER,
  CCI_TELEMETRY_LOCATION_FOOTER,
} cci_telemetry_location_t;

#define WAIT_FOR_BUSY_DEASSERT() while (cci_read_register(fd, CCI_REG_STATUS) & 0x01) ;

/* Setup */
int cci_init(int fd);

/* Primative methods */
int cci_write_register(int fd, uint16_t reg, uint16_t value);
uint16_t cci_read_register(int fd, uint16_t reg);

/* Module: SYS */
void cci_run_ffc(int fd);
uint32_t cci_get_uptime(int fd);
void cci_set_telemetry_enable_state(int fd, cci_telemetry_enable_state_t state);
uint32_t cci_get_telemetry_enable_state(int fd);
void cci_set_telemetry_location(int fd, cci_telemetry_location_t location);
uint32_t cci_get_telemetry_location(int fd);

#endif /* CCI_H */
