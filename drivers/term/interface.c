#include <driver.h>
#include <termdrv.h>
#include <errno.h>
#include <vtt.h>

struct nirux_interface_driver terminal_interface = {
    .name = "term",
    .restrict_fd_access = 0,
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
    // TODO!

    return -ENOSUP;
}