#include <errno.h>
#include <device.h>
#include <string.h>

static inline struct nirux_device_node *get_node(uint16_t addr) {
    return is_device_node_set(addr / 256, addr % 256) ? get_device_node(addr / 256) : NULL;
}

int dinfo(uint16_t *seek, struct nirux_device_info *dev) {
    for (uint16_t offset = 0; (*(seek) + offset) <= (256 * 256) - 1; offset++) {
        uint8_t major = (*(seek) + offset) / 256;
        uint8_t minor = (*(seek) + offset) % 256;

        struct nirux_device_node *node = get_device_node(major);
        if (node && is_device_node_set(major, minor)) {
            memset(dev, 0, sizeof(*dev));

            if (node->iface->modctl) node->iface->modctl(0xf0000000 | (minor << 8) | major, dev->name);

            dev->major = major;
            dev->minor = minor;

            strcpy(dev->driver, (char *) node->iface->name);

            *seek += offset + 1;

            return 0;
        }
    }

    return -1;
}

ssize_t dread(uint16_t addr, size_t *seek, void *restrict buf, size_t n) {
    struct nirux_device_node *node = get_node(addr);
    if (node) {
        ssize_t r = node->iface->read(addr / 256, addr % 256, seek ? *seek : 0, buf, n);
        if (r > 0 && seek) *seek += r;

        return r;
    }

    return -ENXIO;
}

ssize_t dwrite(uint16_t addr, size_t *seek, const void *buf, size_t n) {
    struct nirux_device_node *node = get_node(addr);
    if (node) {
        ssize_t w = node->iface->write(addr / 256, addr % 256, seek ? *seek : 0, buf, n);
        if (w > 0 && seek) *seek += w;

        return w;
    }

    return -ENXIO;
}

int dchmod(uint16_t addr, uint16_t mode) {
    struct nirux_device_node *node = get_node(addr);
    int64_t owner;
    if (node && (owner = get_device_node_owner(addr / 256, addr % 256) >= 0)) {
        return restrict_device_node(addr / 256, addr % 256, owner & 0xffff, (owner >> 16) & 0xffff, mode);
    }

    return -ENXIO;
}

int dchown(uint16_t addr, uint16_t uid, uint16_t gid) {
    struct nirux_device_node *node = get_node(addr);
    int32_t mode;
    if (node && (mode = get_device_node_perm(addr / 256, addr % 256) >= 0)) {
        return restrict_device_node(addr / 256, addr % 256, uid, gid, mode & 0xffff);
    }

    return -ENXIO;
}

int dioctl(uint16_t addr, int op, void *arg) {
    struct nirux_device_node *node = get_node(addr);
    return node ? node->iface->ioctl(addr / 256, addr % 256, op, arg) : -ENXIO;
}