#include <irq.h>
#include <fbdev.h>
#include <stdio.h>
#include <termdrv.h>
#include <vtt.h>
#include <kmem.h>
#include <uname.h>
#include <timer.h>
#include <device.h>

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    // register syscall
    register_syscall();

    // initialise interfaces
    video_interface.init(NULL);
    terminal_interface.init(NULL);

    // initialise vga console driver
    vga_console_init();

    // initialise virtual terminal
    if(vtt_initialise(4)) return;
    puts("// Initialised virtual terminal driver\n");
    
    printf("// Nirux %s %s\n", UNAME_M, UNAME_R);
    printf("// %d KB total available memory\n", (meminfo.total - meminfo.reserved) / 1024);
    printf("// %d KB used memory\n", meminfo.used / 1024);

    uint16_t seek = 0;
    struct nirux_device_info dev = {{0}, {0}, 0, 0};
    printf("// Detected devices:\n");
    while (dinfo(&seek, &dev) != -1) {
        printf("// %u:%u            %s\n", (uint32_t) dev.major, (uint32_t) dev.minor, dev.driver);
    }

    printf("// Kernel initialised, %u.%u ms elapsed\n", (uint32_t) timer_elapsed, (uint32_t) timer_elapsed_f);
}