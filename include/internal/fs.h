#pragma once

#include <driver.h>
#include <stdint.h>

#define MAXPATHSIZ 512
#define MAXFSMNTED 16

enum filesystem_direntry_type {
    // types
    FS_FILE     = 0x0,
    FS_DIR      = 0x1,
    FS_SYMLINK  = 0x2,
    FS_HRDLINK  = 0x4,
    FS_FIFO     = 0x8,
    FS_NODE     = 0x10,
    FS_CTL      = 0x20,

    // attributes
    FS_HIDDEN   = 0x0100,
};

struct filesystem_direntry {
    uint16_t type;
    uint32_t perm;
    uint16_t uid, gid;
    uint64_t last_access, size, ref, parent_ref;

    char filename[];
} __attribute__((__packed__));

struct filesystem_driver {
    char name[DRVNAMLEN + 1];

    int (*initialise)(uint16_t device);

    ssize_t (*fetch_entry)(
        uint16_t device,
        struct filesystem_direntry *restrict entry,
        size_t *seek,
        const char *path
    );

    ssize_t (*read)(
        uint16_t device,
        const struct filesystem_direntry *entry,
        void *restrict buf,
        size_t seek,
        size_t n
    );

    ssize_t (*write)(
        uint16_t device,
        const struct filesystem_direntry *entry,
        const void *buf,
        size_t seek,
        size_t n
    );

    int (*creat)(
        uint16_t device,
        const struct filesystem_direntry *dir,
        const char *name,
        uint16_t type,
        const void *arg
    );

    int (*unlink)(
        uint16_t device,
        const struct filesystem_direntry *entry
    );

    int (*chmod)(
        uint16_t device,
        const struct filesystem_direntry *entry,
        uint32_t mode
    );

    int (*chown)(
        uint16_t device,
        const struct filesystem_direntry *entry,
        uint16_t uid, uint16_t gid
    );
} __attribute__((__packed__));

struct filesystem {
    char mountpoint[MAXPATHSIZ + 1];
    uint16_t device;
    ssize_t depends_on;

    struct filesystem_driver *driver;
} __attribute__((__packed__));

extern struct nirux_interface_driver fs_interface;