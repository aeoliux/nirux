#include <x86floppy.h>
#include <irq.h>
#include <fbdev.h>
#include <stdio.h>
#include <termdrv.h>
#include <vtt.h>
#include <kmem.h>
#include <uname.h>
#include <timer.h>
#include <device.h>
#include <block.h>
#include <ioport.h>

extern int x86floppy_perform_command(uint8_t command, ...);

extern uint8_t *x86floppy_dma;

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    // register syscall
    register_syscall();

    // initialise interfaces
    pseudo_interface.init(NULL);
    video_interface.init(NULL);
    terminal_interface.init(NULL);
    block_interface.init(NULL);

    // initialise vga console driver
    vga_console_init();

    // initialise virtual terminal
    if(vtt_initialise(4)) return;
    puts("// Initialised virtual terminal driver\n");

    int floppy_ready = !block_floppy_driver.probe();

    printf("// Nirux %s %s\n", UNAME_M, UNAME_R);
    printf("// %d KB total memory, %d KB reserved\n", meminfo.total / 1024, meminfo.reserved / 1024);
    printf("// %d KB used memory\n", meminfo.used / 1024);

    uint16_t seek = 0;
    struct nirux_device_info dev = {{0}, {0}, 0, 0};
    printf("// Detected devices:\n");
    while (dinfo(&seek, &dev) != -1) {
        printf("//      %s: /dev/%s (%u:%u)\n", dev.driver, dev.name, (uint32_t) dev.major, (uint32_t) dev.minor);
    }

    printf("// Kernel initialised, %u.%u ms elapsed\n", (uint32_t) timer_elapsed, (uint32_t) timer_elapsed_f);

    if (floppy_ready) {
        printf("x86floppy reading 1st drive\n");
        uint8_t sector[2048];
        ssize_t result = block_interface.read(8, 0, 0, sector, 2048);
        printf("x86floppy read result: %d\n", result);
        if (result >= 1) {
            puts("FAT12 label: ");
            dwrite(0, 0, sector + 0x2B, 11);
            putchar('\n');

            puts("FAT12 oem:   ");
            dwrite(0, 0, sector + 0x3, 8);
            putchar('\n');
        }
        printf("x86floppy reading 2nd drive\n");
        result = block_interface.read(8, 1, 0, sector, 2048);
        printf("x86floppy read result: %d\n", result);
        if (result >= 1) {
            puts("FAT12 label: ");
            dwrite(0, 0, sector + 0x2B, 11);
            putchar('\n');

            puts("FAT12 oem:   ");
            dwrite(0, 0, sector + 0x3, 8);
            putchar('\n');
        }
    }
}
