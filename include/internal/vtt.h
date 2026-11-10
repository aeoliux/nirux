#pragma once

#include <termdrv.h>

struct nirux_vtt {
    uint8_t direct_render;
    uint16_t cur_x, cur_y;
    uint16_t buffer[];
} __attribute__((__packed__));

extern struct nirux_terminal_driver virtual_terminal_driver;

extern uword_t vtt_current;
extern uint16_t vtt_width, vtt_height;
extern uint16_t *direct_render;
extern struct nirux_vtt **vtt_ptrs;

ssize_t vtt_read(uint8_t minor, void *restrict buf, size_t n);
ssize_t vtt_write(uint8_t minor, const void *buf, size_t n);
int vtt_ioctl(uint8_t minor, int op, void *arg);
int vtt_initialise(size_t spawn);

ssize_t vtt_buffer_append_str(struct nirux_vtt *vtt, uint8_t color, const uint8_t *src, size_t n, uint8_t direct);
ssize_t vtt_buffer_append_char(struct nirux_vtt *vtt, uint8_t color, uint8_t ch, uint8_t direct);