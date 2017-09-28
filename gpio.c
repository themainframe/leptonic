#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "gpio.h"

/**
 * Export a pin for use in our code.
 */
int gpio_export(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;

	fd = open(GPIO_ROOT "export", O_WRONLY);
	if (-1 == fd) {
		perror("GPIO: failed to open export for writing\n");
		return(-1);
	}

	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

/**
 * Unexport a pin.
 */
int gpio_unexport(int pin)
{
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	fd = open(GPIO_ROOT "unexport", O_WRONLY);
	if (-1 == fd) {
		perror("GPIO: failed to open unexport for writing!\n");
		return(-1);
	}
	bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
	write(fd, buffer, bytes_written);
	close(fd);
	return(0);
}

int gpio_direction(int pin, int dir)
{
	static const char s_directions_str[]  = "in\0out";
	char path[DIRECTION_MAX];
	int fd;

	snprintf(path, DIRECTION_MAX, GPIO_ROOT "gpio%d/direction", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		perror("GPIO: failed to open gpio direction for writing!\n");
		return(-1);
	}

	if (-1 == write(fd, &s_directions_str[GPIO_IN == dir ? 0 : 3], GPIO_IN == dir ? 2 : 3)) {
		perror("GPIO: failed to set direction!\n");
		return(-1);
	}

	close(fd);
	return(0);
}

/**
 * Read the state of a pin.
 */
int gpio_read(int pin)
{
	char path[VALUE_MAX];
	char value_str[3];
	int fd;

	snprintf(path, VALUE_MAX, GPIO_ROOT "gpio%d/value", pin);
	fd = open(path, O_RDONLY);
	if (-1 == fd) {
		perror("GPIO: failed to open gpio value for reading!\n");
		return(-1);
	}

	if (-1 == read(fd, value_str, 3)) {
		perror("GPIO: failed to read value!\n");
		return(-1);
	}

	close(fd);

	return(atoi(value_str));
}

/**
 * Write the state of a pin.
 */
int gpio_write(int pin, int value)
{
	static const char s_values_str[] = "01";
	char path[VALUE_MAX];
	int fd;

	snprintf(path, VALUE_MAX, GPIO_ROOT "gpio%d/value", pin);
	fd = open(path, O_WRONLY);
	if (-1 == fd) {
		perror("GPIO: failed to open gpio value for writing\n");
		return(-1);
	}

	if (1 != write(fd, &s_values_str[GPIO_LOW == value ? 0 : 1], 1)) {
		perror("GPIO: failed to write value\n");
		return(-1);
	}

	close(fd);
	return(0);
}
