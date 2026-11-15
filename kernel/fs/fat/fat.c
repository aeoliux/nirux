#include <fs.h>
#include <fs/fat.h>
#include <device.h>
#include <errno.h>

ssize_t fs_fat_current_fat_sector = -1;

int fs_fat_read_fat(
    struct fs_fat_superblock *block,
    size_t fat_sector
) {
    if (fs_fat_current_fat_sector == fat_sector)
        return 0;

    size_t seek = fat_sector * block->sector_size;
    ssize_t result = dread(
        block->device,
        &fat_sector,
        block->buffer,
        (block->fat_type & FS_12BIT) ? 2 * block->sector_size : block->sector_size
    );
    if (result < 0) return result;
    
    fs_fat_current_fat_sector = fat_sector;

    return 0;
}

uint32_t fs_fat_cluster(
    struct fs_fat_superblock *block,
    size_t current_cluster
) {
    size_t fat_offset;
    size_t fat_sector;
    size_t ent_offset;

    if (block->fat_type & FS_FAT16) {
        // for F12 and F16
        fat_offset = (block->fat_type & FS_12BIT)
            ? (current_cluster / 2 + current_cluster)
            : (current_cluster * 2);
        fat_sector = fat_offset / block->sector_size + block->reserved_sectors;
        ent_offset = fat_offset % block->sector_size;
    } else {
        // for F32
        return -ENOSUP;
    }

    int result = fs_fat_read_fat(block, fat_sector);
    if (result) return result;

    uint32_t entry = (block->fat_type & FS_FAT16)
        ? (*(uint16_t *) &block->buffer[ent_offset]) & 0xffff
        : *(uint32_t *) &block->buffer[ent_offset];

    if (block->fat_type & FS_12BIT)
        entry = (entry & 1) ? entry >> 4 : entry & 0xfff;
    else if (block->fat_type & FS_FAT32)
        entry &= 0x0fffffff;

    return entry;
}