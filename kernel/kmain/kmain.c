#include <irq.h>
#include <fbdev.h>
#include <stdio.h>
#include <termdrv.h>
#include <vtt.h>
#include <kmem.h>
#include <uname.h>
#include <timer.h>

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    // register syscall
    register_syscall();

    // initialise vga console driver
    vga_console_init();

    // initialise virtual terminal
    if(vtt_initialise(4)) return;
    puts("// Initialised virtual terminal driver\n");
    
    printf("// Nirux %s %s\n", UNAME_M, UNAME_R);
    printf("// %d KB total available memory\n", (meminfo.total - meminfo.reserved) / 1024);
    printf("// %d KB used memory\n", meminfo.used / 1024);

    printf("// Kernel initialised, %u.%u ms elapsed\n", (uint32_t) timer_elapsed);
}