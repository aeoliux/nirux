#include <device.h>
#include <errno.h>
#include <modctl.h>
#include <block.h>
#include <driver.h>
#include <stdio.h>
#include <string.h>

#define PARTBEGIN 132

size_t block_devices_counter = 0, block_partitions_counter = 0;
struct nirux_block_device block_devices[BLOCK_MAX_DEV];

void part_probe() {
    struct mbr mbr;

    block_partitions_counter = 0;
    for (size_t i = 0; i < PARTBEGIN && i < block_devices_counter && block_partitions_counter < 256 - 132; i++) {
        if (!block_devices[i].driver || block_devices[i].type != BLOCK_DRIVE)
            continue;

        struct nirux_block_driver *drv = (struct nirux_block_driver *) block_devices[i].driver;
        if (drv->read(&block_devices[i], 0, &mbr, 1) < 512 || mbr.magic != 0xaa55)
            continue;
    
        for (size_t j = 0; j < 4; j++) {
            struct mbr_entry *p = &mbr.part[j];
            if ((p->active && p->active != 0x80) || !p->total_sectors || !p->start_lba)
                continue;

            uint8_t daddr = 132 + block_partitions_counter;

            struct nirux_block_device part = {
                .addr = block_devices[i].addr,
                .type = BLOCK_PARTITION | BLOCK_DRIVE | (j << 8),
                .removable = block_devices[i].removable,
                .sector_size = block_devices[i].sector_size,
                .total_lba = p->total_sectors,
                .driver = block_devices[i].driver,
                .reserved = (void *) p->start_lba,
            };

            memcpy(&block_devices[daddr], &part, sizeof(struct nirux_block_device));

            populate_device_node(9, daddr);
            restrict_device_node(9, daddr, 0, GID_BLOCK, 0660);
            populate_device_node(8, daddr);
            restrict_device_node(8, daddr, 0, GID_RAWDISK, 0660);

            block_partitions_counter++;

            if (daddr > block_devices_counter)
                block_devices_counter = daddr;
        }
    }
}

int block_modctl(uword_t op, void *arg) {
    uint16_t addr;

    switch (op) {
        case MODCTLRELOAD:
            return -ENOSUP;
        case BLOCK_MODCTL_REGISTER:
            addr = ((struct nirux_block_device *) arg)->addr;

            if (block_devices_counter + 1 < BLOCK_MAX_DEV && !is_device_node_set(9, addr % 256) && !is_device_node_set(8, addr % 256)) {
                memcpy(&block_devices[addr % 256], arg, sizeof(struct nirux_block_device));

                int result = populate_device_node(9, addr % 256);
                if (!result) return result;

                restrict_device_node(9, addr % 256, 0, GID_BLOCK, 0660);

                result = populate_device_node(8, addr % 256);
                if (!result) return result;

                restrict_device_node(8, addr % 256, 0, GID_RAWDISK, 0660);

                if (addr % 256 > block_devices_counter)
                    block_devices_counter = (addr % 256) + 1;

                if (block_devices[addr % 256].type == BLOCK_DRIVE) part_probe();

                return 0;
            }

            return -ENOMEM;
        default:
            if (op & 0xf0000000) {
                uint8_t major = op & 0xff;
                uint8_t minor = (op >> 8) & 0xff;

                struct nirux_block_device *dev = (struct nirux_block_device *) &block_devices[minor];
                memcpy(arg, ((struct nirux_block_driver *) dev->driver)->name, DRVNAMLEN + 1);
                size_t length = strlen(arg);
                if (length < (DRVNAMLEN - 5) && major == 9) {
                    strncat(arg, "blk", 3);
                    length = strlen(arg);
                }

                if (length < DRVNAMLEN - 2) {
                    if (dev->type & BLOCK_PARTITION)
                        minor = dev->addr % 256;

                    i32toa(arg + length, (uint32_t) minor, 10, 1);

                    if (dev->type & BLOCK_PARTITION) {
                        char p = 'p';
                        strncat(arg, &p, 1);

                        uint8_t pnum = (dev->type >> 8) & 0xff;
                        i32toa(arg + strlen(arg), (uint32_t) pnum, 10, 1);
                    }
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
