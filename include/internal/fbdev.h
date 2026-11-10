#pragma once

#include <driver.h>

#define MODFBDEVNOTIF 0x0001
#define IOFBDEVMODSET 0x0002
#define IOFBDEVMOVCUR 0x0004
#define IOFBDEVTXTSET 0x0008
#define IOFBDEVGETCUR 0x0010
#define IOFBDEVGETRES 0x0020
#define IOFBDEVBFADDR 0x0040
#define IOFBDEVTXSCRL 0x0080

struct iofbdevtxtset_arg {
    size_t seek;
    size_t count;
    uint16_t entry;
} __attribute__((__packed__));

#define NFBDEV_OUTPUTNAM_SIZ 8
#define NFBDEV_CURMODE_DISBLD 0x00
#define NFBDEV_CURMODE_TEXTBF 0x01
#define NFBDEV_CURMODE_LINEAR 0x02
#define NFBDEV_CURMODE_DRMFBF 0x04

struct nirux_framebuffer_output {
    uint8_t name[NFBDEV_OUTPUTNAM_SIZ + 1];
    uint8_t current_mode, bits_per_color;
    uint16_t width, height;
    uint16_t cursor_x, cursor_y; // only for text mode is mandatory
    void *buffer;
} __attribute__((__packed__));

#define NFBDEV_MAX_OUTPUTS 1
#define NFBDEV_FLAG_TEXTONLY 0x0000
#define NFBDEV_FLAG_KMODESET 0x0001
#define NFBDEV_FLAG_DRMMDSET 0x0002

struct nirux_framebuffer_driver {
    uint8_t name[DRVNAMLEN + 1];    // for VGA console: vgafb,      for Intel gpus: intelhd/i915
    uint16_t flag;                  // for VGA console: 0x0/0x1,  for Intel gpus: 0x1 | 0x2 = 0x3

    uint8_t outputs_count;          // for VGA console: 1,          for Intel gpus: min(max outputs, number of displays connected)
    struct nirux_framebuffer_output outputs[NFBDEV_MAX_OUTPUTS];

    // only for text mode
    int (*cursor_move)(uint8_t output, uint16_t x, uint16_t y);
    int (*scroll)(uint8_t output, size_t by);

    // can be NULL if not modesettable driver
    int (*modeset)(uint8_t output, uint32_t *restrict size);

    // while textmode:  write - writes text to the gpu text buffer (VGA console format compatibility required - macros are below)
    //                  set - sets n bytes at specific offset to byte
    //                  set16 - sets n bytes at specific offset to word (used by terminal driver)
    //                  set16single - sets specified offset to word
    // while modeset:   write - writes buffer to the vram (linear framebuffer format)
    //                  set - sets n bytes at the vram to n after selected offset
    //                  set16 and set16single (NOT USED BY MODESET!)
    ssize_t (*write)(uint8_t output, size_t seek, const void *buf, size_t n);
    ssize_t (*set)(uint8_t output, size_t seek, uint8_t byte, size_t n);
    ssize_t (*set16)(uint8_t output, size_t seek, uint16_t entry, size_t n);
    ssize_t (*set16single)(uint8_t output, size_t seek, uint16_t entry);
} __attribute__((__packed__));

extern struct nirux_interface_driver video_interface;

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

#define VGA_ENTRY_COLOR(fg, bg) ((fg) | (bg) << 4)
#define VGA_ENTRY(ch, color) ((uint16_t) (ch) | ((uint16_t) (color) << 8))
#define VGA_INDEX(x, y, width) ((y) * (width) + (x))
#define VGA_ENTRY_CHAR VGA_ENTRY(' ', VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK))

void vga_console_init();