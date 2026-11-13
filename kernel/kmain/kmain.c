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
    video_interface.init(NULL);
    terminal_interface.init(NULL);
    block_interface.init(NULL);

    // initialise vga console driver
    vga_console_init();

    // initialise virtual terminal
    if(vtt_initialise(4)) return;
    puts("// Initialised virtual terminal driver\n");

    if (!block_floppy_driver.probe()) {
        printf("// x86floppy reading\n");
        uint8_t sector[512];
        ssize_t result = block_interface.read(8, 0, 0, sector, 2048);
        printf("// x86floppy read result: %d\n", result);
        if (result >= 1) {
            puts("FAT12 label: ");
            terminal_interface.write(4, 0, 0, sector + 0x2B, 11);
            putchar('\n');

            puts("FAT12 oem:   ");
            terminal_interface.write(4, 0, 0, sector + 0x3, 8);
            putchar('\n');
        }
    }

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
