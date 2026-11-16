#include <fs.h>
#include <fs/fat.h>
#include <device.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int fs_fat_read_fat(
    struct fs_fat_superblock *block,
    size_t fat_sector
) {
    if (block->current_buffered_sector && block->current_buffered_sector == fat_sector)
        return 0;

    size_t seek = fat_sector * block->sector_size;
    ssize_t result = dread(
        block->device,
        &seek,
        block->buffer + 512,
        (block->fat_type & FS_12BIT) ? 2 * block->sector_size : block->sector_size
    );
    if (result < 0) return result;
    
    block->current_buffered_sector = fat_sector;

    return 0;
}

int64_t fs_fat_cluster(
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
    if (result < 0) return result;

    uint32_t entry = (block->fat_type & FS_FAT16)
        ? (*(uint16_t *) &block->buffer[ent_offset + 512]) & 0xffff
        : *(uint32_t *) &block->buffer[ent_offset + 512];

    // uint32_t entry;
    // printf("%x %x", (uint32_t) block->buffer[512], (uint32_t) block->buffer[513]);
    // uint16_t *f16addr = (uint16_t *) ((uint8_t *) block->buffer + 512 + ent_offset);
    // if (block->fat_type & FS_FAT16)
    //     entry = (uint32_t) (*f16addr);

    if (block->fat_type & FS_12BIT)
        entry = (current_cluster & 1) ? entry >> 4 : entry & 0xfff;
    else if (block->fat_type & FS_FAT32)
        entry &= 0x0fffffff;

    return entry;
}

int64_t fs_fat_read_cluster(
    struct fs_fat_superblock *blk,
    size_t cluster,
    size_t sector_index
) {
    if (sector_index >= blk->sectors_per_cluster)
        return -EINVAL;

    int64_t next_i64 = fs_fat_cluster(blk, cluster);
    if (next_i64 <= 0) return next_i64;
    uint32_t next = next_i64;

    uint32_t bad_cluster_sign = 0xfffffff7;
    if          (blk->fat_type & FS_12BIT)      bad_cluster_sign &= 0xfff;
    else if     (blk->fat_type & FS_FAT16)      bad_cluster_sign &= 0xffff;
    if (next == bad_cluster_sign) {
        printf("// fat attempted to read bad cluster - %u on device %x\n", cluster, blk->device);
        return -EIO;
    }

    size_t cluster_sector =
        blk->reserved_sectors + (blk->sectors_per_fat * blk->fats_count) + (blk->root_dir_entries * 32 / blk->sector_size) + ((cluster - 2) * blk->sectors_per_cluster);
    size_t cluster_offset = (cluster_sector + sector_index) * blk->sector_size;

    // printf("%u: %u\n", next, cluster_sector);
    blk->current_buffered_sector = 0;
    ssize_t result = dread(blk->device, &cluster_offset, &blk->buffer[512], blk->sector_size);
    if (result < 0)
        return result;

    return next;
}

ssize_t fs_fat_read(
    uint16_t device,
    const struct fs_direntry *dir,
    void *restrict buf,
    size_t seek,
    size_t n
) {
    ssize_t ind = fs_get_index_for_device(device);
    if (ind < 0)
        return ind;

    struct fs_fat_superblock *blk = &fs_fat_superblocks[ind];

    if (!dir->ref)
        return 0;

    size_t read_limit = seek + n;
    if (read_limit > dir->size) {
        if (seek > dir->size)
            return -EINVAL;
        
        n = dir->size - seek;
    }

    uint32_t cluster = dir->ref;

    uint32_t last_cluster_sign = 0xfffffff8;
    if          (blk->fat_type & FS_12BIT)      last_cluster_sign &= 0xfff;
    else if     (blk->fat_type & FS_FAT16)      last_cluster_sign &= 0xffff;

    size_t seeked_sec = 0;
    int64_t seeked_cluster = cluster;
    while (seek / blk->sector_size > seeked_sec) {
        seeked_cluster = fs_fat_cluster(blk, seeked_cluster);

        if (seeked_cluster < 0)
            return seeked_cluster;

        if (seeked_cluster >= last_cluster_sign)
            return 0;

        if (seeked_cluster == 0) {
            printf("// fat detected corrupted filesystem on %x: file refers to empty cluster\n", device);
            return -EIO;
        }

        seeked_sec += blk->sectors_per_cluster;
    }
    cluster = seeked_cluster;

    seek %= blk->sector_size;

    size_t done = 0;
    int64_t next = 0;
    while ((cluster < last_cluster_sign || seek) && n > 0) {
        size_t sector_index = 0;
        
        while (sector_index < blk->sectors_per_cluster && n > 0) {
            if (cluster < last_cluster_sign) next = fs_fat_read_cluster(blk, cluster, sector_index);
            
            if (next < 0)
                return cluster;

            if (next == 0) {
                printf("// fat detected corrupted filesystem on %x: file refers to empty cluster\n", device);
                return -EIO;
            }

            if (n / blk->sector_size) {
                memcpy(buf + done, &blk->buffer[512 + seek], blk->sector_size - seek);
                done += blk->sector_size - seek;
                n -= blk->sector_size - seek;
            } else {
                size_t off = n % blk->sector_size;
                memcpy(buf + done, &blk->buffer[512], off);
                done += off;
                n = 0;
            }

            seek = 0;
            sector_index++;
        }

        cluster = next;
    }

    return done;
}