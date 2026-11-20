#pragma once

#include <device.h>
#include <fs.h>
#include <fcntl.h>

struct file_descriptor {
    uint8_t isset;

    char path[MAXPATHSIZ + 1];

    int access_mode;
    size_t seek;

    struct fs_driver *drv;
    struct fs *mnt;
    struct fs_direntry ref;
} __attribute__((__packed__));