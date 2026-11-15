#include <errno.h>
#include <fs.h>
#include <fs/fat.h>
#include <device.h>
#include <stdio.h>
#include <string.h>

ssize_t fs_fat_fetch_entry(
    uint16_t device,
    const struct fs_direntry *parent,
    struct fs_direntry *restrict entry,
    size_t *seek
) {
    ssize_t i = fs_get_index_for_device(device);
    if (i < 0)
        return i;

    struct fs_fat_superblock *block = &fs_fat_superblocks[i];

    size_t entry_addr;
    if (!parent && (block->fat_type & FS_FAT16)) {
        if (*seek >= block->root_dir_entries)
            return 0;


        entry_addr =
            (block->sectors_per_fat * block->fats_count + block->reserved_sectors) * block->sector_size
            + (32 * *seek);
    } else if (!parent && (block->fat_type & FS_FAT32)) {
        return -ENOSUP;
    } else {
        return -ENOSUP;
    }

    struct fs_fat_direntry *dir = (struct fs_fat_direntry *) &block->buffer[1030];
    int result = dread(device, &entry_addr, dir, 64);
    if (result < 64)
        return (result < 0) ? result : -EIO;
    
    if ((dir->attr & 0xf) == 0xf) {
        dir = &dir[1];
        (*seek)++;
    }

    if (dir->attr & 0x8) {
        (*seek)++;
        return fs_fat_fetch_entry(device, parent, entry, seek);
    }

    if (!(dir->attr & 0x20))
        return 0;
    
    entry->uid = 0;
    entry->gid = 0;
    entry->perm = 0755;
    entry->parent_ref = parent ? parent->ref : 0;
    entry->type = (dir->attr & 0x10) ? FS_DIR : FS_FILE;
    entry->last_access = 0;
    entry->size = dir->size;
    entry->ref = (block->fat_type & FS_FAT16) ? dir->cluster_low : ((dir->cluster_high << 16) | dir->cluster_low);
    memcpy(entry->filename, dir->name, 11);

    (*seek)++;

    return *seek;
}