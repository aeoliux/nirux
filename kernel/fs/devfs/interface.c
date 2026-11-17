#include <fs.h>
#include <device.h>
#include <errno.h>
#include <string.h>

int devfs_initialise(uint16_t device) {
    return 0;
}

ssize_t devfs_fetch_entry(
    uint16_t device,
    const struct fs_direntry *parent,
    struct fs_direntry *restrict entry,
    size_t *seek
) {
    struct nirux_device_info dev;
    if (dinfo((uint16_t *) seek, &dev))
        return 0;

    int64_t owner = get_device_node_owner(dev.major, dev.minor);
    int32_t mode = get_device_node_perm(dev.major, dev.minor);

    entry->gid = (owner >> 16) & 0xffff;
    entry->uid = owner & 0xffff;
    entry->last_access = 0;
    entry->ref = dev.major * 256 + dev.minor;
    entry->parent_ref = 0;
    entry->mode = mode;
    entry->size = 0;
    entry->type = FS_NODE;

    memset(entry->filename, 0, sizeof entry->filename);
    strcpy(entry->filename, dev.name);

    return *seek;
}

ssize_t devfs_read(
    uint16_t device,
    const struct fs_direntry *entry,
    void *restrict buf,
    size_t seek,
    size_t n
) {
    if (entry->type & ~FS_NODE)
        return -EINVAL;

    return dread(entry->ref, &seek, buf, n);
}

ssize_t devfs_write(
    uint16_t device,
    const struct fs_direntry *entry,
    const void *buf,
    size_t seek,
    size_t n
) {
    if (entry->type & ~FS_NODE)
        return -EINVAL;

    return dwrite(entry->ref, &seek, buf, n);
}

struct fs_driver fs_devfs = {
    .name = "devfs",
    .initialise = devfs_initialise,
    .fetch_entry = devfs_fetch_entry,
    .read = devfs_read,
    .write = devfs_write,
    .creat = NULL,
    .unlink = NULL,
    .chmod = NULL,
    .chown = NULL
};