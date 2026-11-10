#include <fbdev.h>
#include <errno.h>

int vga_modeset(uint8_t output, uint32_t *restrict size) {
    return -ENOSUP; // currently not supported
}