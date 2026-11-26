#include "panic.h"
#include "stdint.h"
#include <fs.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
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
#include <ata.h>

// static inline void msleep(size_t ms) {
//     size_t target = timer_elapsed + ms;
//     while (timer_elapsed < target) suspend();
// }

static void list_dir_recursively(const char *path, size_t margin) {
    struct fs_direntry entry;
    size_t seek = 0;
    ssize_t result;
    char buf[MAXPATHSIZ + 1];

    while ((result = fs_read_dir(path, &seek, &entry)) > 0) {
        for (size_t i = 0; i < margin; i++)
            putchar('-');
        printf("/%s - %u:%u, %o, %u KB\n", entry.filename, (uint32_t) entry.uid, (uint32_t) entry.gid, (uint32_t) entry.mode, (uint32_t) entry.size / 1024);

        if (entry.type & FS_DIR) {
            memset(buf, 0, sizeof buf);
            strcpy(buf, path);

            strcat(buf, entry.filename);

            list_dir_recursively(buf, strlen(buf));
        }
    }

    if (!margin) printf("Done with result = %d\n", result);
}

// static uint8_t task_finished = 0;
// static void simple_task() {
//     // printf("Listing / recursively:\n");
//     // list_dir_recursively("/", 0);
    

//     task_finished = 1;
//     exit(0);
// }

struct ssb_header *ssb_payload = NULL;
size_t ssb_payload_size = 0;

void kmain() {
    //
    // ONLY BASIC TESTS HERE
    //

    void *ret = __builtin_return_address(0);

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

    block_isaata.probe();

    printf("// Nirux %s %s\n", UNAME_M, UNAME_R);
    printf("// %d KB total memory\n", (meminfo.total - meminfo.reserved) / 1024);
    printf("// %d KB used\n", (meminfo.used - meminfo.reserved) / 1024);

    uint16_t seek = 0;
    struct nirux_device_info dev = {{0}, {0}, 0, 0};
    printf("// Detected devices:\n");
    while (dinfo(&seek, &dev) != -1) {
        if (!dev.major || dev.major == 9) continue;
        printf("//      %s: /dev/%s (%u:%u)\n", dev.driver, dev.name, (uint32_t) dev.major, (uint32_t) dev.minor);
    }

    printf("// Kernel initialised, %u.%u ms elapsed\n", (uint32_t) timer_elapsed, (uint32_t) timer_elapsed_f);

    int result = 0;
    if ((result = fs_mount(0, "/dev", &fs_devfs)) < 0)
        panic("couldn't mount /dev: %d", ret, result);
    printf("// Mounted devfs on /dev\n");

    const char *argv[] = {"init", NULL};
    if (ssb_payload_size && ssb_payload) {
        printf("// Executing ssb payload as init script\n");
        if ((result = exec_ssb(SSB_SRC_MEM, (uword_t) ssb_payload, argv, NULL, NULL)) < 0)
            panic("payload execution failed, exec_ssb = %d", ret, result);
    } else if (floppy_ready) {
        printf("// Attempting to mount /dev/isaata128p0 (FAT) as /\n");
        if ((result = fs_mount_root(0x984, &fs_fat)))
            panic("couldn't mount /dev/isaata128p0, fs_mount = %d", ret, result);

        list_dir_recursively("/", 0);

        int fd;
        printf("// /init\n");
        if ((fd = open("/init", O_EXEC)) < 0)
            panic("couldn't execute /init, open = %d", ret, fd);

        if ((result = exec_ssb(SSB_SRC_FILE, (uword_t) fd, argv, NULL, NULL)) < 0)
            panic("/init execution failed, exec_ssb = %d", ret, result);
    } else panic("no /", ret);

    while (1) suspend();
}
