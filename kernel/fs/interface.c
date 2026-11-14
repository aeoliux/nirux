#include <device.h>
#include <driver.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fs.h>

extern struct nirux_interface_driver fs_interface;

ssize_t fs_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    // implement read

    return -ENOSUP;
}

ssize_t fs_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {
    // implement write

    return -ENOSUP;
}

int fs_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {
    // implement ioctl commands

    return -ENOSUP;
}

int fs_modctl(uword_t op, void *arg) {
    // switch (op) {
    //     // implement mod control commands

    //     default:
    //         if (op & 0xf0000000) {
    //             uint8_t major = op & 0xff;
    //             uint8_t minor = (op >> 8) & 0xff;

    //             memcpy(arg, "fs", 4);
    //             size_t length = strlen(arg);
    //             if (length < DRVNAMLEN - 2) {
    //                 i32toa(arg + strlen(arg), (uint32_t) minor, 10, 1);
    //             }
                
    //             return 0;
    //         }
    // }

    return -EINVAL;
}

int fs_init(void *arg) {
    return 0;
}

struct nirux_interface_driver fs_interface = {
    .name = "pseudo",
    .init = fs_init,
    .read = fs_read,
    .write = fs_write,
    .ioctl = fs_ioctl,
    .modctl = fs_modctl,
};
