#include <device.h>
#include <errno.h>
#include <modctl.h>
#include <block.h>
#include <driver.h>
#include <stdio.h>
#include <string.h>

size_t block_devices_counter = 0;
struct nirux_block_device block_devices[BLOCK_MAX_DEV] = {0};

int block_modctl(uword_t op, void *arg) {
    uint16_t addr;

    switch (op) {
        case MODCTLRELOAD:
            return -ENOSUP;
        case BLOCK_MODCTL_REGISTER:
            addr = ((struct nirux_block_device *) arg)->addr;

            if (block_devices_counter + 1 < BLOCK_MAX_DEV && !is_device_node_set(9, addr % 256) && !is_device_node_set(8, addr % 256)) {
                memcpy(&block_devices[block_devices_counter++], arg, sizeof(struct nirux_block_device));

                int result = populate_device_node(9, addr % 256);
                if (!result) return result;

                restrict_device_node(9, addr % 256, 0, GID_BLOCK, 0660);

                result = populate_device_node(8, addr % 256);
                if (!result) return result;

                restrict_device_node(8, addr % 256, 0, GID_RAWDISK, 0660);
            }

            return -ENOMEM;
        default:
            if (op & 0xf0000000) {
                uint8_t major = op & 0xff;
                uint8_t minor = (op >> 8) & 0xff;

                memcpy(arg, ((struct nirux_block_driver *) block_devices[minor].driver)->name, DRVNAMLEN + 1);
                size_t length = strlen(arg);
                if (length < (DRVNAMLEN - 5) && major == 9) {
                    strncat(arg, "blk", 3);
                    length = strlen(arg);
                }

                if (length < DRVNAMLEN - 2) {
                    i32toa(arg + length, (uint32_t) minor, 10, 1);
                }
                
                return 0;
            }
    }

    return -EINVAL;
}

int block_init(void *arg) {
    int res = claim_device_node(8, &block_interface);
    return res ? res : claim_device_node(9, &block_interface);
}

ssize_t block_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    if (minor > block_devices_counter || block_devices[minor].type == 0x0)
        return -ENXIO;

    struct nirux_block_device *blk = &block_devices[minor];
    struct nirux_block_driver *drv = (struct nirux_block_driver *) blk->driver;

    return (major == 9) ? block_read_block(blk, drv, seek, buf, n) : block_read_bytes(blk, drv, seek, buf, n);
}

ssize_t block_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {
    return -ENOSUP;
}

int block_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {
    return -ENOSUP;
}

struct nirux_interface_driver block_interface = {
    .name = "block",

    .init = block_init,
    .read = block_read,
    .write = block_write,
    .ioctl = block_ioctl,
    .modctl = block_modctl
};
