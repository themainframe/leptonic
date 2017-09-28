#ifndef GPIO_H
#define GPIO_H
#define GPIO_ROOT "/sys/class/gpio/"
#define GPIO_IN  0
#define GPIO_OUT 1
#define GPIO_LOW  0
#define GPIO_HIGH 1
#define DIRECTION_MAX 35
#define BUFFER_MAX 3
#define VALUE_MAX 30

int gpio_export(int pin);
int gpio_unexport(int pin);
int gpio_direction(int pin, int dir);
int gpio_read(int pin);
int gpio_write(int pin, int value);

#endif /* SPIDEV_H */
