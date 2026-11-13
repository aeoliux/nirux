#include <block.h>
#include <driver.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

struct nirux_block_floppy_driver *block_floppy_drivers[] = {
    #if defined (__i386__)
    &x86floppy_driver
    #endif
};
const size_t block_floppy_drivers_count = sizeof(block_floppy_drivers) / sizeof(struct nirux_block_floppy_driver);
size_t block_floppy_drives_count = 0;

int block_floppy_probe() {
    uint32_t prober[BLOCK_MAX_FLOPPY_DRV];
    block_floppy_drives_count = 0;
    for (size_t i = 0; i < block_floppy_drivers_count && block_floppy_drives_count < BLOCK_MAX_FLOPPY_DRV; i++) {
        struct nirux_block_floppy_driver *driver = block_floppy_drivers[i];

        ssize_t probed = 0;
        if ((probed = driver->probe(prober, BLOCK_MAX_FLOPPY_DRV - block_floppy_drives_count)) > 0) {
            for (size_t j = 0; j < probed; j++) {
                uint32_t drive_type = prober[j];

                struct nirux_block_device device = {
                    .addr = 8 * 256 + block_floppy_drives_count,
                    .type = ((block_floppy_drives_count & 0xff) << 16 | drive_type | BLOCK_FLOPPY) & 0xffffff,
                    .removable = 1,
                    .last_lba = 0,
                    .driver = &block_floppy_driver,
                    .reserved = driver,
                };

                if (block_interface.modctl(BLOCK_MODCTL_REGISTER, &device) != -1) block_floppy_drives_count++;
            }
        }
    }

    return 0;
}

ssize_t block_floppy_write(uint32_t drive, size_t seek, const void *buf, size_t n) {
    return -ENOSUP;
}

ssize_t block_floppy_read(uint32_t drive, size_t seek, void *restrict buf, size_t n) {
    return -ENOSUP;
}

struct nirux_block_driver block_floppy_driver = {
    .name = "fd",
    .probe = block_floppy_probe,
    .write = block_floppy_write,
    .read = block_floppy_read
};