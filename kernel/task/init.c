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
    .pid = 0,
    .parent = -1,
    .state = TASK_RUNNING,
    .substate = 0,
    .is_kthread = 1,
    .pagedir = 0,
    .kstack = 0,
    .env_section = 0,
    .user = 0xffffffff,
    .sections = {}
};

struct task *current_task = &ktask;
struct task *last_task = &ktask;

int tasking_init() {
    arch_setup_ktask();
    memset(ktask.fds, 0, MAXFDS * 2);

    pidtable_init();

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

    struct task *t = kalloc(2);
    if (!t) {
        if (ustack) kfreen((void *) ustack, ustacksiz);
        if (kstack) kfreen((void *) kstack, STACKSIZ);

        return NULL;
    }
    
    memset((void *) ustack, 0, ustacksiz * meminfo.pagesize);
    memset((void *) kstack, 0, STACKSIZ * meminfo.pagesize);
    memset(t, 0, sizeof (struct task));

    t->parent = is_kthread ? 0 : current_task->pid;
    t->is_kthread = is_kthread;
    t->pagedir = is_kthread ? ktask.pagedir : 0;
    t->kstack = kstack;
    t->ustack = ustack;
    t->ustacksiz = is_kthread ? 0 : ustacksiz;
    t->user = flags;
    t->euser = flags;

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

            kfreen(t, 2);

            return NULL;
        }
    }

    arch_task_init_stack(t, entry, 0);

    // printf("%x %x %u\n", kstack, t->sp, t->kstack_size);
    // __asm__ volatile ("cli; hlt");

    t->state = TASK_READY;

    if (pidtable_register(t) < 0) {
        close_at_task(t, 0);
        close_at_task(t, 1);
        close_at_task(t, 2);

        if (ustack) kfreen((void *) ustack, ustacksiz);
        if (kstack) kfreen((void *) kstack, STACKSIZ);

        kfreen(t, 2);

        return NULL;
    }

    if (!current_task->is_kthread && !is_kthread) {
        t->pgid = current_task->pgid;
        t->sid = current_task->sid;
    } else if (!is_kthread) {
        t->pgid = t->pid;
        t->sid = t->sid;
    }

    return t;
}

pid_t task_clone(struct task *t) {
    if (t == &ktask)
        return -EPERM;

    size_t sectioned = 0;
    struct task *new = kclone(t, 2);
    if (!new)
        return -ENOMEM;
    memset(new->childs, 0, new->childs_count * sizeof(pid_t));
    new->childs_count = 0;
    new->sig_pending = 0;
    new->state = 0;
    new->pagedir = 0;
    
    uword_t ustack = (uword_t) kclone((void *) t->ustack, t->ustacksiz);
    if (!ustack) { kfree(new); return -ENOMEM; }

    uword_t kstack = (uword_t) kclone((void *) t->kstack, STACKSIZ);
    if (!kstack) { kfreen((void *) ustack, t->ustacksiz); kfree(new); return -ENOMEM; }

    new->parent = t->pid;
    new->kstack = kstack;
    new->ustack = ustack;

    for (size_t i = 0; i < t->occupied; i++) {
        if (new->sections[i].stack) {
            new->sections[i].ptr = ustack;
        } else {
            new->sections[i].ptr = (uword_t) kclone((void *) t->sections[i].ptr, t->sections[i].n);
            if (!new->sections[i].ptr)
                goto error;
        }
        sectioned++;
    }

    new->pagedir = arch_setup_pages(new->sections, new->occupied);
    if (!new->pagedir)
        goto error;

    arch_task_prepare_copied_stack(new, (uword_t) 0);

    pid_t pid = pidtable_register(new);
    if (pid < 0)
        goto error;

    new->state = t->state;

    return pid;
error:
    for (size_t j = 0; j < sectioned; j++)
        kfreen((void *) new->sections[j].ptr, new->sections[j].n);

    kfree((void *) new->pagedir);
    kfreen((void *) new->ustack, new->ustacksiz);
    kfreen((void *) new->kstack, STACKSIZ);
    kfreen(new, 2);
    return -ENOMEM;
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