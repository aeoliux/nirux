#include <device.h>
#include <driver.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

extern struct nirux_interface_driver new_interface;

ssize_t new_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    // implement read

    return -ENOSUP;
}

ssize_t new_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {
    // implement write

    return -ENOSUP;
}

int new_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {
    // implement ioctl commands

    return -ENOSUP;
}

int new_modctl(uword_t op, void *arg) {
    switch (op) {
        // implement mod control commands

        default:
            if (op & 0xf0000000) {
                uint8_t major = op & 0xff;
                uint8_t minor = (op >> 8) & 0xff;

                memcpy(arg, "new", 4);
                size_t length = strlen(arg);
                if (length < DRVNAMLEN - 2) {
                    i32toa(arg + strlen(arg), (uint32_t) minor, 10, 1);
                }
                
                return 0;
            }
    }

    return -EINVAL;
}

int new_init(void *arg) {
    return claim_device_node(*((uint8_t *) arg), &new_interface);
}

struct nirux_interface_driver new_interface = {
    .name = "pseudo",
    .init = new_init,
    .read = new_read,
    .write = new_write,
    .ioctl = new_ioctl,
    .modctl = new_modctl,
};
