/*
 * host_watchdog.c -- Checks if slaves are alive or not.
 *
 * To build and run this program, install libi2c-dev.
 */

#include "planetproj.h"
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>

#define printf_exit(str, ...) \
    do { \
        fprintf(stderr, "%s:%d (%s): ", __FILE__, __LINE__, __func__); \
        fprintf(stderr, str, ##__VA_ARGS__); \
        putc('\n', stderr); \
        exit(EXIT_FAILURE); \
    } while (0)

#define pprintf_exit(str, ...) \
    do { \
        fprintf(stderr, "%s:%d (%s): ", __FILE__, __LINE__, __func__); \
        fprintf(stderr, str, ##__VA_ARGS__); \
        fprintf(stderr, ": %s\n", strerror(errno)); \
        exit(EXIT_FAILURE); \
    } while (0)

/*
 * wiringPi is a famous library to do these stuffs, but it does not export GPIO
 * pins even if the user belongs to gpio group, so we decided to manipulate
 * sysfs directly.
 */

static void pin_export(const unsigned pin)
{
    const char *filename = "/sys/class/gpio/export";
    const int fd = open(filename, O_WRONLY);
    if (fd == -1)
        pprintf_exit("open(%s)", filename);

    dprintf(fd, "%u", pin);

    const int err = close(fd);
    if (err)
        pprintf_exit("close(%s)", filename);
}

static void pin_set_output(const unsigned pin)
{
    char filename[PATH_MAX];
    snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%u/direction",
            pin);

    const int fd = open(filename, O_WRONLY);
    if (fd == -1)
        pprintf_exit("open(%s)", filename);

    const ssize_t ret = write(fd, "out", 3);
    if (ret != 3)
        pprintf_exit("write(%s)", filename);

    const int err = close(fd);
    if (err)
        pprintf_exit("close(%s)", filename);
}

static void pin_output(const unsigned pin, const _Bool val)
{
    char filename[PATH_MAX];
    snprintf(filename, sizeof(filename), "/sys/class/gpio/gpio%u/value", pin);

    const int fd = open(filename, O_WRONLY);
    if (fd == -1)
        pprintf_exit("open(%s)", filename);

    const ssize_t ret = write(fd, val ? "1" : "0", 1);
    if (ret != 1)
        pprintf_exit("write(%s)", filename);

    const int err = close(fd);
    if (err)
        pprintf_exit("close(%s)", filename);
}

static void check_i2c_funcs(const int fd)
{
    unsigned long funcs;
    int err;

    err = ioctl(fd, I2C_FUNCS, &funcs);
    if (err)
        pprintf_exit("ioctl(I2C_FUNCS)");

    if (!(funcs & I2C_FUNC_I2C))
        printf_exit("I2C is not supported?");

    if (!(funcs & I2C_FUNC_SMBUS_QUICK))
        printf_exit("Quick protocol is not supported");
}

#define NUM_SLAVES 4

int main(int argc, char *argv[])
{
    int fd;
    int err;
    const char *filename = argv[1];
    const struct slave {
        unsigned long addr;
        unsigned pin_red;
        unsigned pin_green;
    } slaves[NUM_SLAVES] = {
        {ADDR_LED_1,   22, 10},
        {ADDR_LED_2,   11,  5},
        {ADDR_MOTOR_1,  6, 13},
        {ADDR_MOTOR_2, 19, 26},
    };

    if (argc != 1 + 1)
        printf_exit("Usage: %s I2CDEV", argv[0]);

    fd = open(filename, O_NONBLOCK);
    if (fd == -1)
        pprintf_exit("open: %s", filename);

    check_i2c_funcs(fd);

    for (size_t i = 0; i < NUM_SLAVES; i ++) {
        const unsigned pin_red = slaves[i].pin_red;
        const unsigned pin_green = slaves[i].pin_green;
        pin_export(pin_red);
        pin_export(pin_green);
    }

    /* Sleep for udev to relax permission of gpio files. */
    (void) nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = 100000000L},
            NULL);

    for (size_t i = 0; i < NUM_SLAVES; i ++) {
        const unsigned long addr = slaves[i].addr;
        const unsigned pin_red = slaves[i].pin_red;
        const unsigned pin_green = slaves[i].pin_green;
        int32_t res;

        err = ioctl(fd, I2C_SLAVE, addr);
        if (err) {
            if (errno != EBUSY)
                pprintf_exit("ioctl(I2C_SLAVE, %lu)", addr);

            printf("Slave 0x%02x is busy (maybe alive)\n", addr);
            res = 0;
        } else
            res = i2c_smbus_write_quick(fd, I2C_SMBUS_WRITE);

        pin_set_output(pin_red);
        pin_set_output(pin_green);

        if (res >= 0) {
            printf("Slave 0x%02x is alive\n", addr);
            pin_output(pin_red, 0);
            pin_output(pin_green, !0);
        } else {
            printf("Slave 0x%02x is dead\n", addr);
            pin_output(pin_red, !0);
            pin_output(pin_green, 0);
        }
    }

    err = close(fd);
    if (err)
        pprintf_exit("close");

    return 0;
}
