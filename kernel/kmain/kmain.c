#include <fs.h>
#include <unistd.h>
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
#include <task.h>
#include <ssb.h>

// static inline void msleep(size_t ms) {
//     size_t target = timer_elapsed + ms;
//     while (timer_elapsed < target) suspend();
// }

// static void list_dir_recursively(const char *path, size_t margin) {
//     struct fs_direntry entry;
//     size_t seek = 0;
//     ssize_t result;
//     char buf[MAXPATHSIZ + 1];

//     while ((result = fs_read_dir(path, &seek, &entry)) > 0) {
//         for (size_t i = 0; i < margin; i++)
//             putchar('-');
//         printf("/%s - %u:%u, %o, %u KB\n", entry.filename, (uint32_t) entry.uid, (uint32_t) entry.gid, (uint32_t) entry.mode, (uint32_t) entry.size / 1024);

//         if (entry.type & FS_DIR) {
//             memset(buf, 0, sizeof buf);
//             strcpy(buf, path);

//             strcat(buf, entry.filename);

//             list_dir_recursively(buf, strlen(buf));
//         }
//     }

//     if (!margin) printf("Done with result = %d\n", result);
// }

// static uint8_t task_finished = 0;
// static void simple_task() {
//     // printf("Listing / recursively:\n");
//     // list_dir_recursively("/", 0);
    

//     task_finished = 1;
//     exit(0);
// }

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    // register syscall
    register_syscall();

    tasking_init();

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

    if (floppy_ready && fs_mount_root(0x900, &fs_fat) >= 0) {
        printf("// Mounted /dev/fdblk0 as /\n");
        if (fs_mount(0, "/dev", &fs_devfs) >= 0)
            printf("// Mounted devfs on /dev\n");

        printf("Executing /bin/a.ssb..\n");
        struct task *t = read_ssb(current_task, "/bin/a.ssb");
        if (t) t->state = TASK_RUNNING;

        while (t->state & TASK_RUNNING) { suspend(); }

        printf("Task finished!\n");
    }
}
