#include <device.h>
#include <driver.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <kmem.h>

enum pseudo_node_type {
    PSEUDO_NODE_MEMORY      = 0x08000000,
    PSEUDO_NODE_CONSTANT    = 0x10000000,
    PSEUDO_NODE_DROP        = 0x20000000,
    PSEUDO_NODE_RANDOM      = 0x40000000,
    PSEUDO_NODE_REDIRECT    = 0x80000000
};

struct pseudo_node {
    char name[DRVNAMLEN + 1];
    uint32_t mode;
};

struct pseudo_node pseudo_nodes[] = {
    {"output",  PSEUDO_NODE_REDIRECT | (4 * 256)},
    {"mem",     PSEUDO_NODE_MEMORY},
    {"null",    PSEUDO_NODE_DROP},
    {"zero",    PSEUDO_NODE_CONSTANT},
    {"random",  PSEUDO_NODE_RANDOM}
};

ssize_t pseudo_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    if (minor > 4)
        return -ENXIO;

    uint32_t limit, mode = pseudo_nodes[minor].mode;
    switch (mode & 0xff000000) {
        case PSEUDO_NODE_REDIRECT:
            return dread(mode & 0xffff, &seek, buf, n);
        case PSEUDO_NODE_CONSTANT:
            memset(buf, mode & 0xff, n);
            return n;
        case PSEUDO_NODE_MEMORY:
            limit = seek + n;
            if (limit >= meminfo.total)
                return -ENOMEM;

            memcpy(buf, (void *) seek, n);
            return n;
        case PSEUDO_NODE_DROP:
            return 0;
    }

    return -ENOSUP;
}

ssize_t pseudo_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {
    if (minor > 4)
        return -ENXIO;

    uint32_t limit, mode = pseudo_nodes[minor].mode;
    switch (mode & 0xff000000) {
        case PSEUDO_NODE_REDIRECT:
            return dwrite(mode & 0xffff, &seek, buf, n);
        case PSEUDO_NODE_CONSTANT:
        case PSEUDO_NODE_RANDOM:
        case PSEUDO_NODE_DROP:
            return n;
        case PSEUDO_NODE_MEMORY:
            return -EPERM;
    }

    return -ENOSUP;
}

int pseudo_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {
    if (minor > 4)
        return -ENXIO;

    uint32_t limit, mode = pseudo_nodes[minor].mode;
    switch (mode & 0xff000000) {
        case PSEUDO_NODE_REDIRECT:
            return dioctl(mode & 0xffff, op, arg);
    }

    return -ENOSUP;
}

int pseudo_modctl(uword_t op, void *arg) {
    switch (op) {
        // implement mod control commands

        default:
            if (op & 0xf0000000) {
                uint8_t major = op & 0xff;
                uint8_t minor = (op >> 8) & 0xff;

                if (minor > 4)
                    return -ENXIO;

                memcpy(arg, pseudo_nodes[minor].name, DRVNAMLEN + 1);
                
                return 0;
            }
    }

    return -EINVAL;
}

int pseudo_init(void *arg) {
    int result = claim_device_node(0, &pseudo_interface);
    if (!result)
        for (size_t i = 0; i < 5; i++)
            populate_device_node(0, i);

    return result;
}

struct nirux_interface_driver pseudo_interface = {
    .name = "pseudo",
    .init = pseudo_init,
    .read = pseudo_read,
    .write = pseudo_write,
    .ioctl = pseudo_ioctl,
    .modctl = pseudo_modctl,
};
