#include <string.h>
#include <termdrv.h>
#include <vtt.h>
#include <fbdev.h>

int vtt_buffer_set_cursor(struct nirux_vtt *vtt, uint16_t x, uint16_t y, uint8_t direct) {
    vtt->cur_x = x;
    vtt->cur_y = y;

    if (direct) {
        uint16_t pos[] = {x, y};
        return video_interface.ioctl(0, 0, IOFBDEVMOVCUR, &pos);
    }

    return 0;
}

int vtt_buffer_scroll_by(struct nirux_vtt *vtt, size_t by, uint8_t direct) {
    if (direct) return video_interface.ioctl(0, 0, IOFBDEVTXSCRL, &by);
    else {
        uint16_t *addr = (uint16_t *) vtt->buffer;

        for (size_t i = 1; i < vtt_height; i++) {
            uint16_t *from = &addr[i * vtt_width], *to = &addr[(i - 1) * vtt_width];
            memcpy(to, from, vtt_width * sizeof(uint16_t));
        }

        memset16(&addr[(vtt_height - 1) * vtt_width], VGA_ENTRY_CHAR, vtt_width);
    }

    return 0;
}

int vtt_buffer_move_cursor_by(struct nirux_vtt *vtt, uint16_t off, uint8_t direct) {
    uint16_t off_x = vtt->cur_x + off;
    uint16_t x = off_x % vtt_width;
    uint16_t y = vtt->cur_y + (off_x / vtt_width);

    if (y >= vtt_height) {
        x = 0;
        y = vtt_height - 1;
    }

    return vtt_buffer_set_cursor(vtt, x, y, direct);
}

ssize_t vtt_buffer_set_str(struct nirux_vtt *vtt, size_t seek, uint8_t color, const uint8_t *src, size_t n, uint8_t direct, size_t *cursor_by) {
    size_t ret = 0;
    *cursor_by = 0;

    struct iofbdevtxtset_arg arg = { .seek = seek, .count = 1, .entry = 0 };
    for (size_t i = 0; i < n; i++) {
        size_t cur = 0;
        switch (src[i]) {
            case '\n':
                cur = seek + *cursor_by;
                *cursor_by += vtt_width - (cur % vtt_width);
                ret++;
                continue;
            case '\b':
                continue;
            case '\r':
                continue;
            case '\t':
                continue;
            default:
                if (direct) {
                    arg.entry = VGA_ENTRY(src[i], color);
                    if (video_interface.ioctl(0, 0, IOFBDEVTXTSET, &arg) != 1)
                        return ret;

                    arg.seek++;
                } else {
                    vtt->buffer[arg.seek + i] = VGA_ENTRY(src[i], color);
                }

                break;
        }

        ret++;
        (*cursor_by)++;
    }

    return ret;
}

ssize_t vtt_buffer_append_str(struct nirux_vtt *vtt, uint8_t color, const uint8_t *src, size_t n, uint8_t direct) {
    size_t seek = VGA_INDEX(vtt->cur_x, vtt->cur_y, vtt_width);
    size_t scrollable = ((seek + n) >= (vtt_width * vtt_height));
    size_t offscroll = scrollable ? (seek + n - (vtt_width * vtt_height)) : 0;
    n -= offscroll;

    size_t cursor_by = 0;
    ssize_t ret = vtt_buffer_set_str(vtt, seek, color, src, n, direct, &cursor_by);
    if (ret >= 1) vtt_buffer_move_cursor_by(vtt, cursor_by, direct);

    if (scrollable)
        vtt_buffer_scroll_by(vtt, 1, direct);

    if (offscroll)
        ret += vtt_buffer_append_str(vtt, color, src + n, offscroll, direct);

    return ret;
}