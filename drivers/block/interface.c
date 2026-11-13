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

            if (block_devices_counter + 1 < BLOCK_MAX_DEV && !is_device_node_set(addr / 256, addr % 256)) {
                memcpy(&block_devices[block_devices_counter++], arg, sizeof(struct nirux_block_device));
                return populate_device_node(addr / 256, addr % 256);
            }

            return -ENOMEM;
    }

    return -EINVAL;
}

int block_init(void *arg) {
    return claim_device_node(8, &block_interface);
}

uint8_t block_read_buffer[4096] = {0};
ssize_t block_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    if (minor > block_devices_counter || block_devices[minor].type == 0x0)
        return -ENXIO;

    struct nirux_block_device *blk = &block_devices[minor];
    struct nirux_block_driver *drv = (struct nirux_block_driver *) blk->driver;

    size_t middle_sectors_start = (seek / blk->sector_size) + ((seek % blk->sector_size) > 0);
    size_t middle_sectors_count = (n / blk->sector_size) - (((n + seek) % blk->sector_size) > 0);
    size_t middle_sectors_limit = middle_sectors_start + middle_sectors_count;

    size_t left_remaining_bytes = (middle_sectors_start * blk->sector_size) - seek;
    size_t rght_remaining_bytes = (seek + n) - (middle_sectors_limit * blk->sector_size);

    ssize_t nread = drv->read(blk, middle_sectors_start, buf + left_remaining_bytes, middle_sectors_count);

    ssize_t left_nread = 0;
    if (left_remaining_bytes) {
        left_nread = drv->read(blk, middle_sectors_start - 1, block_read_buffer, 1);

        memcpy(buf, block_read_buffer + (seek % blk->sector_size), left_remaining_bytes);
    }

    ssize_t rght_nread = 0;
    if (rght_remaining_bytes) {
        rght_nread = drv->read(blk, middle_sectors_limit, block_read_buffer, 1);

        memcpy(buf + (middle_sectors_count * blk->sector_size), block_read_buffer, rght_remaining_bytes);
    }

    return nread * blk->sector_size + left_remaining_bytes + rght_remaining_bytes;
}

struct nirux_interface_driver block_interface = {
    .name = "block",
    .restrict_fd_access = 0,

    .init = block_init,
    .read = block_read,
    .write = NULL,
    .ioctl = NULL,
    .modctl = block_modctl
};
