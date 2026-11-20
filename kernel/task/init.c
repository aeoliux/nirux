#include <kmem.h>
//#include <stdio.h>
#include <string.h>
#include <task.h>

struct task ktask = {
    .state = TASK_RUNNING,
    .substate = 0,
    .is_kthread = 1,
    .pagedir = 0,
    .kstack = 0,
    .user = 0xffffffff,
    .sections = {},
    .next = NULL
};

struct task *current_task = &ktask;
struct task *last_task = &ktask;

int tasking_init() {
    arch_setup_ktask();

    return 0;
}

struct task *task_create(uword_t flags, uword_t entry, uword_t ustacksiz) {
    uword_t ustack = 0, kstack = 0;

    uint8_t is_kthread = ~flags == 0;

    if (!is_kthread)
        if (!(ustack = (uword_t) kalloc(STACKSIZ)))
            return NULL;

    if (!(kstack = (uword_t) kalloc(STACKSIZ))) {
        if (ustack) kfreen((void *) ustack, ustacksiz);

        return NULL;
    }

    struct task *t = kalloc(1);
    if (!t) {
        if (ustack) kfreen((void *) ustack, ustacksiz);
        if (kstack) kfreen((void *) kstack, STACKSIZ);

        return NULL;
    }
    
    memset((void *) ustack, 0, ustacksiz * meminfo.pagesize);
    memset((void *) kstack, 0, STACKSIZ * meminfo.pagesize);
    memset(t, 0, sizeof (struct task));

    t->is_kthread = is_kthread;
    t->pagedir = is_kthread ? ktask.pagedir : 0;
    t->kstack = kstack;
    t->ustack = ustack;
    t->ustacksiz = ustacksiz;
    t->user = flags;

    arch_task_init_stack(t, entry);

    // printf("%x %x %u\n", kstack, t->sp, t->kstack_size);
    // __asm__ volatile ("cli; hlt");

    last_task->next = t;
    last_task = t;

    t->state = TASK_READY;

    return t;
}
