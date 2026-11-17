#pragma once

#include <driver.h>
#include <stdint.h>

#define MAXPATHSIZ 511
#define MAXFNAMSIZ 127
#define MAXFSMNTED 16

enum fs_direntry_type {
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

struct fs_direntry {
    uint16_t type;
    uint16_t uid, gid, mode;
    uint64_t last_access, size, ref, parent_ref;

    char filename[MAXFNAMSIZ + 1];
} __attribute__((__packed__));

struct fs_driver {
    char name[DRVNAMLEN + 1];

    int (*initialise)(uint16_t device);

    ssize_t (*fetch_entry)(
        uint16_t device,
        const struct fs_direntry *parent,
        struct fs_direntry *restrict entry,
        size_t *seek
    );

    ssize_t (*read)(
        uint16_t device,
        const struct fs_direntry *entry,
        void *restrict buf,
        size_t seek,
        size_t n
    );

    ssize_t (*write)(
        uint16_t device,
        const struct fs_direntry *entry,
        const void *buf,
        size_t seek,
        size_t n
    );

    int (*creat)(
        uint16_t device,
        const struct fs_direntry *dir,
        const char *name,
        uint16_t type,
        const void *arg
    );

    int (*unlink)(
        uint16_t device,
        const struct fs_direntry *entry
    );

    int (*chmod)(
        uint16_t device,
        const struct fs_direntry *entry,
        uint32_t mode
    );

    int (*chown)(
        uint16_t device,
        const struct fs_direntry *entry,
        uint16_t uid, uint16_t gid
    );
} __attribute__((__packed__));

struct fs {
    char mountpoint[MAXPATHSIZ + 1];
    uint16_t device;
    ssize_t depends_on;

    struct fs_driver *driver;
} __attribute__((__packed__));

extern struct fs fs_mounts[MAXFSMNTED + 1];
extern struct nirux_interface_driver fs_interface;

int fs_mount_root(uint16_t device, struct fs_driver *fsdrv);
ssize_t fs_get_mountpoint(const char *path, size_t *offset);
ssize_t fs_read_path(const char *path, size_t seek, void *restrict buf, size_t n);
ssize_t fs_read_dir(const char *path, size_t *seek, struct fs_direntry *restrict out);
ssize_t fs_resolve_file(const char *pth, struct fs **mnt, struct fs_direntry *restrict file);

int fs_is_dir(const char *path);
int fs_mount(uint16_t device, const char *path, struct fs_driver *fsdrv);