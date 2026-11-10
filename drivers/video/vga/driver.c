#include <driver.h>
#include <fbdev.h>
#include <string.h>

extern int vga_cursor_move(uint8_t output, uint16_t x, uint16_t y);
extern int vga_modeset(uint8_t output, uint32_t *restrict size);
extern ssize_t vga_set16(uint8_t output, size_t seek, uint16_t entry, size_t n);
extern ssize_t vga_set16_single(uint8_t output, size_t seek, uint16_t entry);
extern ssize_t vga_console_write(uint8_t output, size_t seek, const void *buf, size_t n);
extern int vga_scroll(uint8_t output, size_t by);

ssize_t vga_write(uint8_t output, size_t seek, const void *buf, size_t n);
ssize_t vga_set(uint8_t output, size_t seek, uint8_t byte, size_t n);

struct nirux_framebuffer_driver vga_framebuffer_driver = {
    .name = "vga",
    .flag = NFBDEV_FLAG_TEXTONLY,
    
    .outputs_count = 0,
    .outputs = {
        {
            .name = "VGA0",
            .current_mode =  NFBDEV_CURMODE_TEXTBF,
            .bits_per_color = 4,
            .width = 80,
            .height = 25,
            .cursor_x = 0,
            .cursor_y = 0,
            .buffer = (void *) 0xB8000,
        }
    },

    .cursor_move = vga_cursor_move,
    .scroll = vga_scroll,
    
    .modeset = vga_modeset,

    .write = vga_write,
    .set = vga_set,
    .set16 = vga_set16,
    .set16single = vga_set16_single,
};

ssize_t vga_write(uint8_t output, size_t seek, const void *buf, size_t n) {
    memcpy(vga_framebuffer_driver.outputs[0].buffer + seek, buf, n);

    return n;
}

ssize_t vga_set(uint8_t output, size_t seek, uint8_t byte, size_t n) {
    memset(vga_framebuffer_driver.outputs[0].buffer + seek, byte, n);

    return n;
}