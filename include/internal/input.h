#pragma once

#include <stdint.h>
#include <driver.h>
#include <device.h>

enum nirux_input_device_type {
    INPUT_KYBRD = 0x1,
    INPUT_MOUSE = 0x2,
    INPUT_INAUX = 0x4,
};

struct nirux_input_driver {
    char name[DRVNAMLEN + 1];

    int (*initialise)();
    ssize_t (*await)(uint16_t devaddr, void *restrict buf, size_t n);
} __attribute__((__packed__));

struct nirux_input_device {
    uint16_t type;
    uint8_t *fifo;
    size_t fifosiz;
    struct nirux_input_driver *drv;
} __attribute__((__packed__));

extern struct nirux_interface_driver input_interface;

extern struct nirux_input_device input_kybrd_fifo[16];
extern struct nirux_input_device input_mouse_fifo[16];
extern struct nirux_input_device input_inalt_fifo[16];

extern struct nirux_input_driver input_ps2pc;
