#pragma once

#include <fs.h>
#include <driver.h>

extern struct fs_driver fs_fat;

enum fs_fat_type {
    FS_FAT16            = 0x1,
    FS_12BIT            = 0x2,
    FS_FAT12            = FS_FAT16 | FS_12BIT,
    FS_FAT32            = 0x4
};

struct fs_fat_superblock {
    uint32_t    current_data_buffered_sector;
    uint32_t    current_fat_buffered_sector;
    uint8_t     fat_type;
    uint16_t    device; // here we gonna store device number and FAT
                        // fs type and that fills oem and jump instructions
                        // up to 11 bytes

    uint16_t    sector_size;
    uint8_t     sectors_per_cluster;
    uint16_t    reserved_sectors;
    uint8_t     fats_count;

    uint16_t    root_dir_entries;

    uint16_t    sectors_count_16;
    uint8_t     mdt;
    uint16_t    sectors_per_fat;
    uint16_t    sectors_per_track;
    uint16_t    heads_count;
    
    uint32_t    hidden_sectors_count;
    uint32_t    sectors_count;

    uint8_t     extended_superblock[472];
    uint8_t     *buffer;
} __attribute__((__packed__));

struct fs_fat1x_extended_superblock {
    uint8_t     disk_no; // DO NOT RELY ON THIS
    uint8_t     nt_flags;
    uint8_t     signature;
    uint8_t     volume_serial[4];
    uint8_t     volume_label[11];
    uint8_t     system_id[8];

    uint8_t     _pad[486];
} __attribute__((__packed__));

struct fs_fat_direntry {
    char name[11];
    uint8_t attr;

    uint8_t nt_reserved;

    uint8_t created_secs_f;
    uint16_t created_time;
    uint16_t created_date;
    uint16_t last_accessed;
    uint16_t cluster_high;
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((__packed__));

struct fs_fat_longfname {
    uint8_t seq;
    uint8_t first[10];
    uint8_t attr;
    uint8_t long_type;
    uint8_t checksum;
    uint8_t second[12];
    uint16_t zero;
    uint8_t final[4];
} __attribute__((__packed__));

extern struct fs_fat_superblock *fs_fat_superblocks;
extern struct fs_fat1x_extended_superblock *fs_fat1x_extended_superblocks;
extern size_t fs_fat_initialised;

ssize_t fs_get_index_for_device(uint16_t device);

int64_t fs_fat_cluster(
    struct fs_fat_superblock *block,
    size_t current_cluster
);

int fs_fat_read_fat(
    struct fs_fat_superblock *block,
    size_t fat_sector
);

ssize_t fs_fat_fetch_entry(
    uint16_t device,
    const struct fs_direntry *parent,
    struct fs_direntry *restrict entry,
    size_t *seek
);

int64_t fs_fat_cluster(
    struct fs_fat_superblock *block,
    size_t current_cluster
);

int64_t fs_fat_seek_clusters(
    struct fs_fat_superblock *blk,
    size_t cluster,
    size_t seek_to_byte,
    size_t *seeked_sector
);

int64_t fs_fat_read_cluster(
    struct fs_fat_superblock *blk,
    size_t cluster,
    size_t sector_index
);

ssize_t fs_fat_read(
    uint16_t device,
    const struct fs_direntry *dir,
    void *restrict buf,
    size_t seek,
    size_t n
);