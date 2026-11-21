#include <errno.h>
#include <irq.h>
#include <stdio.h>
#include <task.h>
#include <syscall.h>
#include <device.h>
#include <task.h>
#include <kmem.h>
#include <unistd.h>

uword_t syscall(struct handler_args *args) {
    struct task *t = (struct task *) args->caller;

    switch (args->arg0) {
        case SYS_EXIT:
            printf("Exit code: %d\n", (int) args->arg1);
            task_exit(args->caller, TASK_EXITED, (uint64_t) args->arg1);
            break;
        
        case SYS_WRITE:
            if (args->arg2 < 0x80000000 || args->arg2 > 0x80000000 + (t->occupied * meminfo.pagesize))
                task_exit(t, TASK_FAULT, 0);

            return (uword_t) write(args->arg1, (void *) args->arg2, args->arg3);

        // SPECIAL CALLS

        case SYS_DREAD:
        case SYS_DWRITE:
        case SYS_DIOCTL:
            if (!t->is_kthread && t->user)
                return -EPERM;

            uword_t memlimit = 0x80000000 + (t->occupied * meminfo.pagesize);
            
            if (args->arg3 < 0x80000000 || args->arg3 > memlimit)
                task_exit(t, TASK_FAULT, 0);

            size_t seek = args->arg2;
            switch (args->arg0) {
                case SYS_DREAD: return (uword_t) dread((uint16_t) args->arg1, &seek, (void *) args->arg3, args->arg4);
                case SYS_DWRITE: return (uword_t) dwrite((uint16_t) args->arg1, &seek, (void *) args->arg3, args->arg4);
                case SYS_DIOCTL: return (uword_t) dioctl((uint16_t) args->arg1, (int) args->arg2, (void *) args->arg3);
            }
    }

    return (uword_t) -ENOSUP;
}
