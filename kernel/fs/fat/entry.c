#include "stdint.h"
#include <errno.h>
#include <fs.h>
#include <fs/fat.h>
#include <device.h>
#include <stdio.h>
#include <string.h>

static inline size_t set_fnam(char *dst, const void *s, size_t n) {
    const uint16_t *src = (const uint16_t *) s;

    size_t i = 0;
    for (; i < n && src[i] != 0xffff; i++) {
        uint16_t wch = src[i];
        char ch;

        if ((wch & 0xff) <= 0x7f)
            ch = (char) wch;
        else
            ch = '*';

        dst[i] = ch;
    }

    return i;
}

static inline ssize_t read_sector_for_entry(struct fs_fat_superblock *blk, size_t dir_start, size_t entry, uint8_t is_root) {
    size_t sector, offset;
    if (is_root) {
        if (entry >= blk->root_dir_entries)
            return -EIO;

        sector = dir_start + ((32 * entry) / blk->sector_size);
        offset = (32 * entry) % blk->sector_size;
    } else {
        // to be done for other directories and FAT32
        return -ENOSUP;
    }

    if (!blk->current_buffered_sector || blk->current_buffered_sector != sector) {
        size_t entry_absolute_offset = sector * blk->sector_size;
    
        int result = dread(blk->device, &entry_absolute_offset, &blk->buffer[512], blk->sector_size);
        if (result < blk->sector_size)
            return (result < 0) ? result : -EIO;

        blk->current_buffered_sector = sector;
    }

    return offset / 32;
}

ssize_t fs_fat_fetch_entry(
    uint16_t device,
    const struct fs_direntry *parent,
    struct fs_direntry *restrict entry,
    size_t *seek
) {
    ssize_t ind = fs_get_index_for_device(device);
    if (ind < 0)
        return ind;

    struct fs_fat_superblock *blk = &fs_fat_superblocks[ind];

    size_t root_dir_start;
    if (!parent && (blk->fat_type & FS_FAT16)) {
        if (*seek >= blk->root_dir_entries)
            return 0;

        root_dir_start =
            (blk->sectors_per_fat * blk->fats_count + blk->reserved_sectors);
    } else if (!parent && (blk->fat_type & FS_FAT32)) {
        return -ENOSUP;
    } else {
        return -ENOSUP;
    }

    struct fs_fat_direntry *dir = (struct fs_fat_direntry *) &blk->buffer[512];
    ssize_t offset = read_sector_for_entry(blk, root_dir_start, *seek, blk->fat_type & FS_FAT16);
    if (offset < 0)
        return offset;

    dir = &dir[offset];

    if (
        ((dir->attr & 0xf) != 0xf) && (
            (dir->attr & 0x8) || 
            (blk->fat_type & FS_FAT16 && dir->cluster_low == 0x0) ||
            (dir->cluster_high == 0x0 && dir->cluster_low == 0x0)
        )
    ) {
        (*seek)++;
        return fs_fat_fetch_entry(device, parent, entry, seek);
    }

    int long_fname = 0;
    while ((dir->attr & 0xf) == 0xf) {
        struct fs_fat_longfname *lnam = (struct fs_fat_longfname *) dir;
        dir = &dir[1];
        (*seek)++;
        
        if ((!long_fname && !(lnam->seq & 0x40)) || (lnam->seq & 0x80))
            continue;

        char *dst = entry->filename + ((lnam->seq & ~0xC0) - 1) * 13;
        set_fnam(dst, lnam->first, 5);
        set_fnam(dst + 5, lnam->second, 6);
        set_fnam(dst + 11, lnam->final, 2);

        long_fname++;

        if (((*seek * 32) % blk->sector_size) == 0 && *seek != 0) {
            if ((offset = read_sector_for_entry(blk, root_dir_start, *seek, blk->fat_type & FS_FAT16)) < 0)
                return offset;

            dir = &dir[offset];
        }
    }

    if (!(dir->attr & 0x30))
        return 0;

    entry->uid = 0;
    entry->gid = 0;
    entry->perm = 0755;
    entry->parent_ref = parent ? parent->ref : 0;
    entry->type = (dir->attr & 0x10) ? FS_DIR : FS_FILE;
    entry->last_access = 0;
    entry->size = dir->size;
    entry->ref = (blk->fat_type & FS_FAT16) ? dir->cluster_low : ((dir->cluster_high << 16) | dir->cluster_low);

    if (!long_fname) {
        ssize_t i, j;
        for (i = 7; i >= 0 && dir->name[i] == ' '; i--); // filename offset
        for (j = 0; j < 3 && dir->name[j + 8] != ' '; j++); // extension length

        memcpy(entry->filename, dir->name, i + 1);
        if (j > 0) {
            j++;
            char c = '.';
            memcpy(entry->filename + i + 1, &c, 1);
            memcpy(entry->filename + i + 2, dir->name + 8, j);
            j++;
        }

        entry->filename[i + j] = 0;
    }

    (*seek)++;

    return *seek;
}