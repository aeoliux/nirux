#include <irq.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <kmem.h>
//#include <stdio.h>
#include <string.h>
#include <task.h>
#include <ioport.h>
#include <errno.h>

struct task ktask = {
    .state = TASK_RUNNING,
    .substate = 0,
    .is_kthread = 1,
    .pagedir = 0,
    .kstack = 0,
    .env_section = 0,
    .user = 0xffffffff,
    .sections = {},
    .next = NULL
};

struct task *current_task = &ktask;
struct task *last_task = &ktask;

int tasking_init() {
    arch_setup_ktask();

    memset(ktask.fds, 0, MAXFDS * 2);

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
    t->ustacksiz = is_kthread ? 0 : ustacksiz;
    t->user = flags;

    if ((is_kthread && current_task->is_kthread) || (!is_kthread && !current_task->is_kthread)) {
        memcpy(t->fds, current_task->fds, MAXFDS);

        for (size_t fd = 0; fd < MAXFDS && t->fds[fd * 2]; fd++) {
            struct file_descriptor *f = (struct file_descriptor *) t->fds[fd * 2];
            f->tasks++;
        }
    } else {
        int stdin = open_at_task(t, "/dev/stdin", O_RDONLY, 0);
        int stdout = open_at_task(t, "/dev/stdout", O_WRONLY, 0);
        int stderr = open_at_task(t, "/dev/stderr", O_WRONLY, 0);
        int vtt = open_at_task(t, "/dev/vtt0", O_RDWR, 0);

        if (stdin < 0 || stdout < 1 || stderr < 2 || vtt < 3) {
            close_at_task(t, stdin);
            close_at_task(t, stdout);
            close_at_task(t, stderr);

            if (ustack) kfreen((void *) ustack, ustacksiz);
            if (kstack) kfreen((void *) kstack, STACKSIZ);

            kfree(t);

            return NULL;
        }
    }

    arch_task_init_stack(t, entry, 0);

    // printf("%x %x %u\n", kstack, t->sp, t->kstack_size);
    // __asm__ volatile ("cli; hlt");

    last_task->next = t;
    last_task = t;

    t->state = TASK_READY;

    return t;
}

int task_switch(struct handler_args *args, uword_t entry, size_t envsize) {
    if (current_task == &ktask)
        return -EPERM;

    idisable();

    arch_task_init_stack(current_task, entry, envsize);
    arch_pagedir_set(current_task->pagedir);
    args->stack = current_task->sp;

    return 0;
}