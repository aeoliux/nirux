#pragma once

#include <driver.h>

struct nirux_terminal_driver {
    uint8_t name[DRVNAMLEN + 1];

    size_t terms_count;

    ssize_t (*read)(uint8_t minor, void *restrict buf, size_t n);
    ssize_t (*write)(uint8_t minor, const void *buf, size_t n);
    int (*ioctl)(uint8_t minor, int op, void *arg);
} __attribute__((__packed__));

ssize_t terminal_read(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n);
ssize_t terminal_write(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n);
int terminal_ioctl(uint8_t major, uint8_t minor, int op, void *arg);
int terminal_modctl(uword_t op, void *arg);
int terminal_init(void *arg);

extern struct nirux_interface_driver terminal_interface;