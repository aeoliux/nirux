#include <fbdev.h>
#include <stdio.h>
#include <termdrv.h>
#include <vtt.h>
#include <kmem.h>

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    // initialise vga console driver
    vga_console_init();

    // initialise virtual terminal
    if(vtt_initialise(4)) return;
    puts("kmain.c: initialised virtual terminal driver\n");

    printf("kmain.c: %d KB total available memory\n", (meminfo.total - meminfo.reserved) / 1024);
    printf("kmain.c: %d KB used memory\n", meminfo.used / 1024);
}