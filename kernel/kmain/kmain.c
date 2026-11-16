#include "fs.h"
#include "stdint.h"
#include <string.h>
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
        uint8_t buffer[16384];
        struct fs_direntry entry;
        size_t seek = 0;

        printf("Initialising FAT volume access on /dev/fdblk0\n");
        int result = fs_fat.initialise(0x900);
        printf("Result = %d\n", result);
        if (result >= 0) {
            printf("Reading syslinux.cfg:\n");

            while ((result = fs_fat.fetch_entry(0x900, NULL, &entry, &seek)) > 0) {
                if (!strcmp(entry.filename, "syslinux.cfg"))
                    break;
            }

            if (result) {
                result = fs_fat.read(0x900, &entry, buffer, 0, entry.size);
                if (result > 0)
                    printf("%s\n", buffer);
            }

            printf("Result = %d\n", result);
        }
    }
}
