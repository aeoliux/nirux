#include <fbdev.h>
#include <stdio.h>
#include <termdrv.h>
#include <vtt.h>
#include <kmem.h>
#include <uname.h>

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    // initialise vga console driver
    vga_console_init();

    // initialise virtual terminal
    if(vtt_initialise(4)) return;
    puts("// Initialised virtual terminal driver\n");
    
    printf("// Nirux %s %s\n", UNAME_M, UNAME_R);
    printf("// %d KB total available memory\n", (meminfo.total - meminfo.reserved) / 1024);
    printf("// %d KB used memory\n", meminfo.used / 1024);
}