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

    block_floppy_driver.probe();
    uint32_t floppy_drive = block_devices[0].type;

    size_t target = timer_elapsed + 2000;
    while (timer_elapsed < target) suspend();

    printf("// x86floppy reading\n");
    x86floppy_set_motor(floppy_drive, 1);
    int result = x86floppy_read_dma(0, 0, 1);
    printf("// x86floppy read result: %d\n", result);
    if (!result) {
        puts("FAT12 label: ");
        terminal_interface.write(4, 0, 0, x86floppy_dma + 0x2B, 11);
        putchar('\n');
    }
    x86floppy_set_motor(floppy_drive, 0);
    
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