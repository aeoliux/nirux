#pragma once

#include <stdint.h>

struct fs_uxfat_superblock {
    uint8_t     jmp[3];
    uint8_t     oem[8];

    uint16_t    sector_size;
    uint8_t     sectors_per_cluster;
    uint16_t    reserved_sectors;
    uint8_t     fats_count;
    uint8_t     fat_addressing;     // address_length & 0xf0

    uint32_t    sectors_per_fat;
    
    uint16_t    legacy_spf;         // must be 0
    
    uint64_t    sectors_count;
    uint64_t    seek:56;
    // EXTENDED

    uint8_t     volume_serial[4];
    uint8_t     volume_label[11];
    uint8_t     system_id[8];

    uint8_t     bootcode[512-62];
} __attribute__((__packed__));

enum UXFAT_DIR_ATTR {
    UXFAT_SET       = 0x1,
    UXFAT_SOCKET    = 0x2,
    UXFAT_FIFO      = 0x4,
    UXFAT_VOLUMEID  = 0x8,
    UXFAT_DIR       = 0x10,
    UXFAT_REGULAR   = 0x20,
    UXFAT_HRDLINK   = 0x40,
    UXFAT_SYMLINK   = 0x80,

    UXFAT_STREAM    = 0xf1,
    UXFAT_LFN       = 0x0f,
    UXFAT_LFNSYM    = 0x8f,
};

struct fs_uxfat_longfname {
    uint8_t         attr;
    uint16_t        seq;
    uint8_t         long_type;
    uint8_t         first[28];
} __attribute__((__packed__));

struct fs_uxfat_direntry {
    uint8_t         attr;
    uint64_t        created_ms;
    uint16_t        uid;
    uint8_t         unused;

    uint64_t        accessed_ms;
    uint64_t        modified_ms;

    uint16_t        gid;
    uint16_t        mode;
} __attribute__((__packed__));

struct fs_uxfat_stream {
    uint8_t         attr;
    uint64_t        offset;
    uint64_t        cluster;
    uint64_t        length;
    uint64_t        length_in_clusters:56;
} __attribute__((__packed__));
