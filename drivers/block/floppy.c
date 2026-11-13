#include <block.h>
#include <driver.h>
#include <stdint.h>
#include <stdio.h>

struct nirux_block_floppy_driver *block_floppy_drivers[] = {
    #if defined (__i386__)
    &x86floppy_driver
    #endif
};
const size_t block_floppy_drivers_count = sizeof(block_floppy_drivers) / sizeof(struct nirux_block_floppy_driver *);
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
                    .sector_size = 512,
                    .last_lba = 0,
                    .driver = &block_floppy_driver,
                    .reserved = driver,
                };

                if (block_interface.modctl(BLOCK_MODCTL_REGISTER, &device) != -1) block_floppy_drives_count++;
            }
        }
    }

    return -(block_floppy_drives_count == 0);
}

uint16_t block_floppy_sectors_per_track(uint32_t drive) {
    switch ((drive >> 8) & 0xff) {
        case BLOCK_FLOPPY_3DD:
        case BLOCK_FLOPPY_5DD: return 9;
        case BLOCK_FLOPPY_5HD: return 15;
        case BLOCK_FLOPPY_3HD: return 18;
        case BLOCK_FLOPPY_3ED: return 36;
        default: return 18; // use 3.5" HD 1.44MB as default value
    }
}

struct chs_addr block_floppy_lba_2_chs(uint32_t drive, uint32_t lba) {
    struct chs_addr chs = {0, 0, 0, 0};

    // sectors per track
    chs.spt = block_floppy_sectors_per_track(drive);

    uint16_t temp = 2 * chs.spt;
    // cylinder
    chs.cyl = lba / temp;

    temp = lba % temp;
    // head
    chs.head = temp / chs.spt;
    // sector
    chs.sec = temp % chs.spt + 1;

    return chs;
}

static inline ssize_t block_floppy_perform(struct nirux_block_floppy_driver *driver, uint32_t drive, uint8_t command, size_t seek, void *buf, size_t n) {
    return driver->perform(drive, command, seek, buf, n);
}

ssize_t block_floppy_write(struct nirux_block_device *drive, size_t seek, const void *buf, size_t n) {
    struct nirux_block_floppy_driver *driver = (struct nirux_block_floppy_driver *) drive->reserved;
    return block_floppy_perform(driver, drive->type, BLOCK_FLOPPY_WRITE, seek, (void *) buf, n);
}

ssize_t block_floppy_read(struct nirux_block_device *drive, size_t seek, void *restrict buf, size_t n) {
    struct nirux_block_floppy_driver *driver = (struct nirux_block_floppy_driver *) drive->reserved;
    return block_floppy_perform(driver, drive->type, BLOCK_FLOPPY_READ, seek, buf, n);
}

struct nirux_block_driver block_floppy_driver = {
    .name = "fd",
    .probe = block_floppy_probe,
    .write = block_floppy_write,
    .read = block_floppy_read
};
