#include <kmem.h>
#include <errno.h>
#include <device.h>
#include <fs.h>
#include <fs/fat.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

struct fs_fat_superblock *fs_fat_superblocks = NULL;
struct fs_fat1x_extended_superblock *fs_fat1x_extended_superblocks = NULL;
size_t fs_fat_initialised = 0;

static inline ssize_t fs_fat_kfree_if_empty(ssize_t ret) {
    if (!fs_fat_initialised) {
        kfree(fs_fat_superblocks);
        fs_fat_superblocks = NULL;
        fs_fat1x_extended_superblocks = NULL;
    }

    return ret;
}

ssize_t fs_get_index_for_device(uint16_t device) {
    for (size_t i = 0; i < fs_fat_initialised; i++)
        if (fs_fat_superblocks[i].device == device)
            return i;

    return -ENXIO;
}

int fs_fat_initialise(uint16_t device) {
    if (!fs_fat_superblocks) {
        if ((fs_fat_superblocks = kalloc(4)) == NULL)
            return fs_fat_kfree_if_empty(-ENOMEM);

        fs_fat1x_extended_superblocks = (struct fs_fat1x_extended_superblock *) fs_fat_superblocks->extended_superblock;
    }
    
    if (fs_fat_initialised >= 8)
        return fs_fat_kfree_if_empty(-ENOMEM);

    struct fs_fat_superblock *block = &fs_fat_superblocks[fs_fat_initialised];
    ssize_t n = dread(device, NULL, block, 512);
    if (n < 512)
        return fs_fat_kfree_if_empty(n < 0 ? n : -1);

    if (block->sectors_per_fat) {
        struct fs_fat1x_extended_superblock *ext = &fs_fat1x_extended_superblocks[fs_fat_initialised];
        
        if (!strncmp((char *) ext->system_id, "FAT12", 5))
            block->fat_type = FS_FAT12;
        else if (!strncmp((char *) ext->system_id, "FAT16", 5))
            block->fat_type = FS_FAT16;
        else {
            printf("// fs_fat unknown FAT type on device %x: ", (uint32_t) device);
            dwrite(0, NULL, ext->system_id, 8);
            putchar('\n');
            return fs_fat_kfree_if_empty(-ENOSUP);
        }

        size_t buffer_length = ((2 + block->sectors_per_cluster) * block->sector_size);
        block->buffer = kalloc((buffer_length / meminfo.pagesize) + ((buffer_length % meminfo.pagesize) > 0));
        if (!block->buffer) {
            return fs_fat_kfree_if_empty(-ENOMEM);
        }

        block->device = device;
    } else {
        // FAT32 currently unsupported
        return fs_fat_kfree_if_empty(-ENOSUP);
    }

    block->current_fat_buffered_sector = 0;
    block->current_data_buffered_sector = 0;
    block->sectors_count = block->sectors_count_16 ? block->sectors_count_16 : block->sectors_count;

    return fs_fat_initialised++;
}

struct fs_driver fs_fat = {
    .name = "fat",

    .initialise = fs_fat_initialise,
    .fetch_entry = fs_fat_fetch_entry,
    .read = fs_fat_read
};