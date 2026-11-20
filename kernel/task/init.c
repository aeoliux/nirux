#include <kmem.h>
//#include <stdio.h>
#include <string.h>
#include <task.h>
#include <errno.h>

struct task ktask = {
    .state = TASK_RUNNING,
    .substate = 0,
    .is_kthread = 1,
    .pagedir = 0,
    .kstack = 0,
    .uid = 0,
    .gid = 0,
    .sections = {},
    .next = NULL
};

struct task *current_task = &ktask;
struct task *last_task = &ktask;

int tasking_init() {
    arch_setup_ktask();

    return 0;
}

int task_create(uword_t flags, uword_t entry, uword_t pagedir) {
    uword_t user_sp = 0, kstack = 0;

    uint8_t is_kthread = ~flags == 0;

    if (!is_kthread)
        if (!(user_sp = (uword_t) kalloc(STACKSIZ)))
            return -ENOMEM;

    if (!(kstack = (uword_t) kalloc(STACKSIZ))) {
        if (user_sp) kfreen((void *) user_sp, STACKSIZ);

        return -ENOMEM;
    }

    struct task *t = kalloc(1);
    if (!t) {
        if (user_sp) kfreen((void *) user_sp, STACKSIZ);
        if (kstack) kfreen((void *) kstack, STACKSIZ);

        return -ENOMEM;
    }
    memset(t, 0, sizeof (struct task));

    t->is_kthread = is_kthread;
    t->pagedir = is_kthread ? ktask.pagedir : pagedir;
    t->kstack = kstack;
    t->kstack_size = STACKSIZ * meminfo.pagesize;
    if (!is_kthread) {
        t->uid = flags & 0xffff;
        t->gid = (flags >> 16) & 0xffff;
    }

    arch_task_init_stack(t, entry, user_sp);

    // printf("%x %x %u\n", kstack, t->sp, t->kstack_size);
    // __asm__ volatile ("cli; hlt");

    last_task->next = t;
    last_task = t;

    t->state = TASK_RUNNING;

    return 0;
}
