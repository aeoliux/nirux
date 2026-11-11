#include "modctl.h"
#include <errno.h>
#include <driver.h>
#include <string.h>
#include <termdrv.h>
#include <fbdev.h>
#include <kmem.h>
#include <vtt.h>

struct nirux_terminal_driver virtual_terminal_driver = {
    .name = "vtt",
    .terms_count = 0,

    .read = vtt_read,
    .write = vtt_write,
    .ioctl = vtt_ioctl,
};

uword_t vtt_current = 0;
uint16_t vtt_width = 0, vtt_height = 0;
uint16_t *direct_render = NULL;
struct nirux_vtt **vtt_ptrs = NULL;

int vtt_initialise(size_t spawn) {
    uint16_t scr_size[2], cur_pos[2];
    
    if (video_interface.ioctl(0, 0, IOFBDEVGETCUR, &cur_pos)) return -ENOSUP;
    if (video_interface.ioctl(0, 0, IOFBDEVGETRES, &scr_size)) return -ENOSUP;

    size_t scr_bytes = (vtt_width * vtt_height) * sizeof(uint16_t);
    size_t vtt_terminfo_size = scr_bytes + 2 * sizeof(uint16_t);
    size_t required_pages = (vtt_terminfo_size * spawn + ((spawn + 1) * sizeof(struct nirux_vtt **))) / meminfo.pagesize + 1;
    vtt_ptrs = kalloc(required_pages);
    if (!vtt_ptrs)
        return -ENOMEM;

    vtt_width = scr_size[0];
    vtt_height = scr_size[1];

    memset(vtt_ptrs, 0, (spawn + 1) * sizeof(struct nirux_vtt **));
    struct nirux_vtt *first_vtt = (struct nirux_vtt *) vtt_ptrs + (sizeof(struct nirux_vtt **) * (spawn + 1));
    for (size_t n = 0; n < spawn; n++) {
        struct nirux_vtt *vtt = first_vtt + (n * vtt_terminfo_size);

        vtt->direct_render = 0;
        vtt->cur_x = 0;
        vtt->cur_y = 0;

        memset16(vtt->buffer, VGA_ENTRY_CHAR, vtt_width * vtt_height);

        vtt_ptrs[n] = vtt;
    }

    first_vtt->direct_render = 1;
    first_vtt->cur_x = cur_pos[0];
    first_vtt->cur_y = cur_pos[1];

    vtt_current = 0;
    virtual_terminal_driver.terms_count = spawn;

    terminal_interface.modctl(MODCTLRELOAD, (void *) 4);

    return 0;
}

ssize_t vtt_read(uint8_t minor, void *restrict buf, size_t n) {
    // TODO!

    return -ENOSUP;
}

ssize_t vtt_write(uint8_t minor, const void *buf, size_t n) {
    static uint8_t color = VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    if (minor >= virtual_terminal_driver.terms_count)
        return -EFAULT;

    struct nirux_vtt *vtt = vtt_ptrs[minor];

    return vtt_buffer_append_str(vtt, color, (const uint8_t *) buf, n, minor == vtt_current);
}

int vtt_ioctl(uint8_t minor, int op, void *arg) {
    // TODO!

    return -ENOSUP;
}