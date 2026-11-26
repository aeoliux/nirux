#pragma once

#include <block.h>
#include <irq.h>

enum ISAATA_BASE_REG {
    ISAATA_PRIM_IO = 0x1f0,
    ISAATA_SCND_IO = 0x170,
    ISAATA_PRIM_CT = 0x3f6,
    ISAATA_SCND_CT = 0x376
};

enum ISAATA_OFF_REG {
    ISAATA_REG_DATA = 0x0,
    ISAATA_REG_ERR = 0x1,
    ISAATA_REG_FEATURES = ISAATA_REG_ERR,
    ISAATA_REG_SECCOUNT = 0x2,
    
    // CHS
    ISAATA_REG_CHS_SECTR = 0x3,
    ISAATA_REG_CHS_CYLLO = 0x4,
    ISAATA_REG_CHS_CYLHI = 0x5,
    ISAATA_REG_HDDRV = 0x6,

    // LBA
    ISAATA_REG_LBA_LO = 0x3,
    ISAATA_REG_LBA_MI = 0x4,
    ISAATA_REG_LBA_HI = 0x5,

    ISAATA_REG_STATUS = 0x7,
    ISAATA_REG_CMD = 0x7,

    // CONTROL BASE
    ISAATA_REG_CT_ALTER = 0x0,
    ISAATA_REG_CT_DEVCT = 0x0,
    ISAATA_REG_CT_DEVAD = 0x1
};

enum ATA_ERROR {
    ATA_AMNF    = 0x1,
    ATA_TKZNF   = 0x2,
    ATA_ABRT    = 0x4,
    ATA_MCR     = 0x8,
    ATA_IDNF    = 0x10,
    ATA_MC      = 0x20,
    ATA_UNC     = 0x40,
    ATA_BBK     = 0x80
};

enum ATA_COMMANDS {
    ATA_READ = 0x20,

    ATAPI_IDENTIFY = 0xa1,
    ATA_IDENTIFY = 0xec,
};

struct ata_identified {
    char name[41];
    char serial[21];
    uint8_t type;

    uint8_t addressing;
    uint8_t maxmultiple;
    uint16_t sector_size;
    uint64_t total_lba;
} __attribute__((__packed__));

#define ISAATAHDVAL(val, drive, is_lba) ((is_lba << 6) | (drive << 4) | (val) | 0xa0)

extern struct nirux_block_driver block_isaata;

extern size_t isaata_expect[];
extern uint16_t *isaata_buffers[];
extern uint16_t isaata_status[];
extern uword_t (*isaata_irq[])(struct handler_args *);

int isaata_identify(uint8_t bus, uint8_t drive, struct ata_identified *restrict result);
int isaata_is_errored(uint8_t bus);
int isaata_is_pio_ready(uint8_t bus);

ssize_t isaata_read_sectors(uint8_t bus, uint8_t drive, void *buf, uint32_t lba, size_t nsec, uint8_t is_lba);
