#pragma once

#include <driver.h>
#include <fs.h>

struct nirux_device_node {
    struct nirux_interface_driver *iface;
    uint8_t minor[256 / 8];
    uint16_t restrictions[256 * 3];
} __attribute__((__packed__));

struct nirux_device_node *get_device_node(uint8_t major);

int claim_device_node(uint8_t node, struct nirux_interface_driver *iface);
int unclaim_device_node(uint8_t major);

int populate_device_node(uint8_t major, uint8_t minor);
int depopulate_device_node(uint8_t major, uint8_t minor);

int is_device_node_set(uint8_t major, uint8_t minor);

int restrict_device_node(uint8_t major, uint8_t minor, uint16_t uid, uint16_t gid, uint16_t mode);
int32_t get_device_node_perm(uint8_t major, uint8_t minor);
int64_t get_device_node_owner(uint8_t major, uint8_t minor);

#define NIRUX_FBDEV_NODE 29

#define DEVNAMLEN 20
struct nirux_device_info {
    char name[DRVNAMLEN + 1];
    char driver[DRVNAMLEN + 1];

    uint8_t major, minor;
};

int dinfo(uint16_t *seek, struct nirux_device_info *dev);
ssize_t dread(uint16_t addr, size_t *seek, void *restrict buf, size_t n);
ssize_t dwrite(uint16_t addr, size_t *seek, const void *buf, size_t n);
int dioctl(uint16_t addr, int op, void *arg);
int dchown(uint16_t addr, uint16_t uid, uint16_t gid);
int dchmod(uint16_t addr, uint16_t mode);

enum nirux_gid {
    GID_TERM        = 5,
    GID_BLOCK       = 6,
    GID_RAWDISK     = 7,
    GID_VIDEO       = 27,
    GID_WHEEL       = 100,
};

extern struct nirux_interface_driver pseudo_interface;

extern struct fs_driver fs_devfs;