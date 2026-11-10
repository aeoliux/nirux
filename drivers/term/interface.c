#include <driver.h>
#include <termdrv.h>
#include <errno.h>

struct nirux_interface_driver terminal_interface = {
    .name = "term",
    .restrict_fd_access = 0,
    .read = terminal_read,
    .write = terminal_write,
    .ioctl = terminal_ioctl,
    .modctl = terminal_modctl
};

ssize_t terminal_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n) {
    // TODO!

    return -ENOSUP;
}

ssize_t terminal_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n) {
    // TODO!

    return -ENOSUP;
}

int terminal_ioctl(uint8_t major, uint8_t minor, int op, void *arg) {
    // TODO!

    return -ENOSUP;
}

int terminal_modctl(uword_t op, void *arg) {
    // TODO!

    return -ENOSUP;
}