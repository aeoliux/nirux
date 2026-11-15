#include "fs.h"
#include "stdint.h"
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
#include <fs/fat.h>

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
        struct fs_direntry entry;
        size_t seek = 0;
            
        int result = fs_fat.initialise(0x800);
        if (result >= 0) {
            puts("A: ");
            
            while ((result = fs_fat.fetch_entry(0x800, NULL, &entry, &seek)) > 0) {
                dwrite(0, NULL, entry.filename, 11);
                putchar(' ');
            }

            putchar('\n');
        }

        // result = fs_fat.initialise(0x801);
        // if (result >= 0) {
        //     puts("B: ");
            
        //     while ((result = fs_fat.fetch_entry(0x801, NULL, &entry, &seek)) > 0) {
        //         dwrite(0, NULL, entry.filename, 11);
        //         putchar(' ');
        //     }

        //     putchar('\n');
        // }
    }
}
