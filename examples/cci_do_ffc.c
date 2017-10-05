#include "log.h"
#include "cci.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdlib.h>

/**
 * Main entry point for example.
 */
int main(int argc, char *argv[])
{
  int fd;

  // Check we have enough arguments to work
  if (argc < 2) {
    log_error("Can't start - I2C device file path must be specified");
    exit(-1);
  }

  // Open the I2C device
  log_info("opening I2C device...");
  fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    log_fatal("I2C: failed to open device - check permissions & i2c enabled");
    exit(-1);
  }

  // Perform an FFC
  cci_init(fd);
  cci_run_ffc(fd);

  // Close up
  close(fd);
  return 0;
}
