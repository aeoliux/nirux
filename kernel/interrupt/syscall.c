#include <fs.h>
#include <fcntl.h>
#include <errno.h>
#include <irq.h>
#include <stdio.h>
#include <task.h>
#include <syscall.h>
#include <device.h>
#include <task.h>
#include <kmem.h>
#include <unistd.h>
#include <sys/mount.h>

static inline void check_mem_perm(struct task *t, uword_t addr) {
    if (addr < 0x80000000 || addr > 0x80000000 + (t->occupied * meminfo.pagesize))
        task_exit(t, TASK_FAULT, 0);
}

static inline void only_root(struct task *t) {
    if ((t->user & 0xffff) && (t->user != (uint32_t) ~0))
        task_exit(t, TASK_FAULT, 0xffff);
}

uword_t syscall(struct handler_args *args) {
    struct task *t = (struct task *) args->caller;
    struct fs_driver *fsdrv;

    switch (args->arg0) {
        case SYS_EXIT:
            task_exit(args->caller, TASK_EXITED, (uint64_t) args->arg1);
            break;
        case SYS_FORK:
            return (uword_t) fork();
        case SYS_READ:
            check_mem_perm(t, args->arg2);
            return (uword_t) read((int) args->arg1, (void *) args->arg2, (size_t) args->arg3);
        case SYS_WRITE:
            check_mem_perm(t, args->arg2);
            return (uword_t) write((int) args->arg1, (void *) args->arg2, (size_t) args->arg3);
        case SYS_OPEN:
            return (uword_t) open((void *) args->arg1, (int) args->arg2, (uint32_t) args->arg3);
        case SYS_CLOSE:
            return (uword_t) close((int) args->arg1);

        case SYS_EXECVE:
            return (uword_t) execve((char *) args->arg1, (char *const *) args->arg2, (char *const *) args->arg3, args);

        case SYS_LSEEK:
            return (uword_t) lseek((int) args->arg1, (off_t) args->arg2, (int) args->arg3);

        case SYS_MOUNT:
            return (uword_t) mount((char *) args->arg1, (char *) args->arg2, (char *) args->arg3, (unsigned long) args->arg4, (void *) args->arg5);

        case SYS_DUP:
            return (uword_t) dup((int) args->arg1);

        // NIRUX EXCLUSIVE

        case SYS_FMOUNT:
            return (uword_t) fmount((int) args->arg1, (char *) args->arg2, (char *) args->arg3, (unsigned long) args->arg4, (void *) args->arg5);
        
        // SPECIAL CALLS

        case SYS_DMNTRT:
            only_root(t);
            check_mem_perm(t, args->arg2);
            fsdrv = fs_get_driver((void *) args->arg2);

            return fs_mount_root((uint16_t) args->arg1, fsdrv);
        case SYS_DREAD:
        case SYS_DWRITE:
        case SYS_DIOCTL:
            only_root(t);
            check_mem_perm(t, args->arg3);

            size_t seek = args->arg2;
            switch (args->arg0) {
                case SYS_DREAD: return (uword_t) dread((uint16_t) args->arg1, &seek, (void *) args->arg3, args->arg4);
                case SYS_DWRITE: return (uword_t) dwrite((uint16_t) args->arg1, &seek, (void *) args->arg3, args->arg4);
                case SYS_DIOCTL: return (uword_t) dioctl((uint16_t) args->arg1, (int) args->arg2, (void *) args->arg3);
            }
    }

    return (uword_t) -ENOSUP;
}
