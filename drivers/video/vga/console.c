#include <fbdev.h>
#include <ioport.h>
#include <string.h>

extern struct nirux_framebuffer_driver vga_framebuffer_driver;
extern struct nirux_interface_driver video_interface;

int vga_cursor_move(uint8_t output, uint16_t x, uint16_t y) {
    uint16_t pos = VGA_INDEX(x, y, 80);

    outb(0x3d4, 0xe);
    outb(0x3d5, (pos >> 8) & 0xff);
    outb(0x3d4, 0xf);
    outb(0x3d5, pos & 0xff);

    vga_framebuffer_driver.outputs[0].cursor_x = x;
    vga_framebuffer_driver.outputs[0].cursor_y = y;

    return 0;
}

ssize_t vga_set16(uint8_t output, size_t seek, uint16_t entry, size_t n) {
    memset16(&((uint16_t *) vga_framebuffer_driver.outputs[0].buffer)[seek], entry, n);

    return n * 2;
}

ssize_t vga_set16_single(uint8_t output, size_t seek, uint16_t entry) {
    ((uint16_t *) vga_framebuffer_driver.outputs[0].buffer)[seek] = entry;

    return 2;
}

void vga_console_init() {
    for (uint8_t y = 0; y < VGA_HEIGHT; y++) {
        uint8_t success = 1;

        for (uint8_t x = 0; x < VGA_WIDTH; x++)
            if ((((uint16_t *) vga_framebuffer_driver.outputs[0].buffer)[VGA_INDEX(x, y, VGA_WIDTH)] & 0xff) != ' ')
                success = 0;
        
        if (success) {
            vga_framebuffer_driver.outputs[0].cursor_y = y;
            break;
        }
    }

    if (vga_framebuffer_driver.outputs[0].cursor_y >= VGA_HEIGHT) {
        vga_framebuffer_driver.outputs[0].cursor_y = 0;

        memset16(vga_framebuffer_driver.outputs[0].buffer, VGA_ENTRY_CHAR, VGA_WIDTH * VGA_HEIGHT);

        vga_cursor_move(0, 0, 0);
    } else {
        vga_cursor_move(0, 0, vga_framebuffer_driver.outputs[0].cursor_y);
    }

    vga_framebuffer_driver.outputs_count = 1;
    video_interface.modctl(MODFBDEVNOTIF, NULL);
}