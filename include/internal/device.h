#pragma once

#include <driver.h>

struct nirux_device_node {
    struct nirux_interface_driver *iface;
    uint8_t minor[256 / 8];
} __attribute__((__packed__));

struct nirux_device_node *get_device_node(uint8_t major);

int claim_device_node(uint8_t node, struct nirux_interface_driver *iface);
int unclaim_device_node(uint8_t major);

int populate_device_node(uint8_t major, uint8_t minor);
int depopulate_device_node(uint8_t major, uint8_t minor);

int is_device_node_set(uint8_t major, uint8_t minor);

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

extern struct nirux_interface_driver pseudo_interface;