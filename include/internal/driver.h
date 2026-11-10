#pragma once

#include <stdint.h>

#define DRVNAMLEN 16

/* Driver architecture
Top-level interface provider driver
|-device driver 1
|-device driver 2
|-etc.

Example:
Top-level interface - Input
|-PS/2 driver
|-usb backed input drivers
|-user input devices

Example:
Top-level interface - Terminal
|-virtual
|-serial RS232
|-serial UART TTL

Example:
Top-level interface - Network
|-tuntap
|-intel ethernet card 1
|-mediatek wifi
*/


// Structure defining an interface driver (top-level provider driver)
// Top-level driver manages its subnodes individually
struct nirux_interface_driver {
    // Interface provider name
    uint8_t name[DRVNAMLEN + 1];
    // Restrict only one file descriptor to be able to access specific device node
    uint8_t restrict_fd_access;
    // Read from device node
    ssize_t (*read)(uint8_t major, uint8_t minor, size_t seek, void *restrict buf, size_t n);
    // Write to device node
    ssize_t (*write)(uint8_t major, uint8_t minor, size_t seek, const void *buf, size_t n);
    // Send I/O commands to the device
    int (*ioctl)(uint8_t major, uint8_t minor, int op, void *arg);
    // Optional (can be NULL)! Send commands to interface driver (ex. register device driver)
    int (*modctl)(uword_t op, void *arg);
} __attribute__((__packed__));

int nirux_register_interface_driver(struct nirux_interface_driver *iface);
