#include <fbdev.h>
#include <termdrv.h>

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    // initialise vga console driver
    vga_console_init();

    // initialise virtual terminal
    if(vtt_initialise(4)) return;

    // get cursor position
    uint16_t cursor[2];
    video_interface.ioctl(29, 0, IOFBDEVGETCUR, &cursor);

    // get text buffer resolution
    uint16_t scrsize[2];
    video_interface.ioctl(29, 0, IOFBDEVGETRES, &scrsize);

    // set some characters
    struct iofbdevtxtset_arg arg = {
        .seek = VGA_INDEX(cursor[0], cursor[1], scrsize[0]),
        .count = 10,
        .entry = VGA_ENTRY('/', VGA_ENTRY_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK))
    };
    video_interface.ioctl(29, 0, IOFBDEVTXTSET, &arg);

    // set cursor to free space
    cursor[0] += 10;
    video_interface.ioctl(29, 0, IOFBDEVMOVCUR, &cursor);
}