#pragma once

#include <driver.h>
#include <stdint.h>

#define BLOCK_MODCTL_REGISTER 0x1
#define BLOCK_MAX_FLOPPY_DRV 4
#define BLOCK_MAX_DEV 16

extern struct nirux_interface_driver block_interface;

enum nirux_block_device_type {
    // block device format as
    // bit 0->7           - device type
    // bit 8->23          - drive number defined by device driver
    // for floppy: 8->15  - floppy drive type
    //             16->23 - floppy drive number
    // bit 24->31         - must be 0!
    BLOCK_DRIVE   = 0x01,  // for hard drives over (S)ATA, SCSI or USB interface.
    BLOCK_FLOPPY  = 0x02,  // for floppy disk drives
    BLOCK_OPTICAL = 0x04,  // for optical media drivers (CD, DVD, BD, etc.)
    BLOCK_UNKNOWN = 0x08,  // unknown block device

    // partition block device as
    // bit 0->7   - device type
    // bit 8->23  - partition number
    // bit 24->31 - all set to 1 as a signature for a partition virtual device
    BLOCK_PARTITION = 0xFF000000
};

struct nirux_block_device {
    uint16_t addr; // for partition devices is set to the original drive entry
    uint32_t type; // nirux_block_device_type value
    uint8_t removable;

    uint16_t sector_size;
    uint64_t last_lba;

    void *driver;
    void *reserved;
} __attribute__((__packed__));

struct nirux_block_driver {
    char name[DRVNAMLEN + 1];

    int (*probe)();

    ssize_t (*write)(struct nirux_block_device *drive, size_t seek, const void *buf, size_t n);
    ssize_t (*read)(struct nirux_block_device *drive, size_t seek, void *restrict buf, size_t n);
} __attribute__((__packed__));

extern struct nirux_block_device block_devices[BLOCK_MAX_DEV];

enum nirux_block_floppy_drives_types {
    BLOCK_FLOPPY_NULL,
    BLOCK_FLOPPY_5DD = 0x0100,
    BLOCK_FLOPPY_5HD = 0x0200,
    BLOCK_FLOPPY_3DD = 0x0400,
    BLOCK_FLOPPY_3HD = 0x0800,
    BLOCK_FLOPPY_3ED = 0x1000
};

struct chs_addr {
    uint16_t cyl, head, sec, spt;
} __attribute__((__packed__));

struct chs_addr block_floppy_lba_2_chs(uint32_t drive, uint32_t lba);

extern struct nirux_block_driver block_floppy_driver;

enum nirux_block_floppy_commands {
    BLOCK_FLOPPY_READ = 0x1,
    BLOCK_FLOPPY_WRITE = 0x2,
};

struct nirux_block_floppy_driver {
    ssize_t (*probe)(uint32_t *out, size_t limit);

    ssize_t (*perform)(uint32_t drive, uint8_t command, uint32_t lba, void *restrict buf, size_t n)
};

extern struct nirux_block_floppy_driver x86floppy_driver;
