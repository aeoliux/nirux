#include <modctl.h>
#include <driver.h>
#include <termdrv.h>
#include <errno.h>
#include <vtt.h>
#include <device.h>

struct nirux_interface_driver terminal_interface = {
    .name = "term",
    .restrict_fd_access = 0,
    .init = terminal_init,
    .read = terminal_read,
    .write = terminal_write,
    .ioctl = terminal_ioctl,
    .modctl = terminal_modctl
};

struct nirux_terminal_driver *terminal_drivers[] = { &virtual_terminal_driver };
int terminal_major_nodes[] = { 4 };

static inline ssize_t terminal_find_node(uint8_t major) {
    for (ssize_t i = 0; i < sizeof(terminal_major_nodes) / sizeof(int); i++) {
        if (major == terminal_major_nodes[i])
            return i;
    }

    return -1;
}

ssize_t terminal_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    int index = terminal_find_node(major);
    if (index != -1 && terminal_drivers[index]->terms_count)
        return terminal_drivers[index]->read(minor, buf, n);

    return -ENOSUP;
}

ssize_t terminal_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {
    int index = terminal_find_node(major);
    if (index != -1 && terminal_drivers[index]->terms_count)
        return terminal_drivers[index]->write(minor, buf, n);

    return -ENOSUP;
}

int terminal_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {
    int index = terminal_find_node(major);
    if (index != -1 && terminal_drivers[index]->terms_count)
        return terminal_drivers[index]->ioctl(minor, op, arg);

    return -ENOSUP;
}

int terminal_modctl(uword_t op, void *arg) {
    uint8_t major = 0;
    ssize_t node_id = 0;

    switch (op) {
        case MODCTLRELOAD:
            major = (uint8_t) ((uword_t) arg & 0xff);
            if ((node_id = terminal_find_node(major)) == -1)
                return -ENXIO;

            struct nirux_terminal_driver *term = terminal_drivers[node_id];
            for (ssize_t i = 0; i < term->terms_count; i++)
                if (populate_device_node(major, i) == -1)
                    return -1;
            
            return 0;
    }

    return -ENOSUP;
}

int terminal_init(void *arg) {
    for (size_t i = 0; i < sizeof(terminal_major_nodes) / sizeof(int); i++)
        if (claim_device_node(terminal_major_nodes[i], &terminal_interface))
            return -1;
    
    return 0;
}