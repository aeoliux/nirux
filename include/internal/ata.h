#pragma once

#include <block.h>

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

enum ATA_COMMANDS {
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

int isaata_identify(uint8_t bus, uint8_t drive, struct ata_identified *restrict result);
