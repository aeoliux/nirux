#include <signal.h>
#include <ioport.h>
#include <kmem.h>
#include <irq.h>
#include <regs.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <timer.h>
#include <panic.h>

static inline struct task *find_next_alive() {
    uword_t *ip = __builtin_return_address(0);

    struct task *startedWith = current_task, *ptr = current_task;
    for (pid_t pid = (startedWith->pid + 1) % (last_pid + 1); pid != startedWith->pid; pid = (pid + 1) % (last_pid + 1)) {
        ptr = pidtable_task(pid);

        switch (ptr->state) {
            case TASK_RUNNING: return ptr;
            case TASK_SLEEP:
                if (ptr->substate < timer_elapsed) {
                    ptr->substate = 0;
                    ptr->state = TASK_RUNNING;
                    return ptr;
                }
                break;
            case TASK_EXITED:
            case TASK_FAULT:
            case TASK_KILLED:
                task_release_mem(ptr);
                break;
        }
    }

    if (ptr->state != TASK_RUNNING) panic("no active tasks: %x", ip, (uint32_t) ptr->state);

    return ptr;
}

void task_msleep(struct task *t, uword_t ms) {
    t->state = TASK_SLEEP;
    t->substate = timer_elapsed + ms;
}

void task_exit(struct task *t, uint16_t state, uint64_t substate) {
    if (!t || t == &ktask)
        return;

    if (state == TASK_FAULT)
        printf("// FAULT: %x\n", (uword_t) substate);

    t->state = state;
    t->substate = substate;

    kill(t->parent, SIGCHLD);

    if (t->sid == t->pid) {
        for (pid_t cpid = 2; cpid < last_pid + 1; cpid++) {
            struct task *c = pidtable_task(cpid);
            if (c && c->sid == t->sid)
                kill(cpid, SIGHUP);
        }
    }

    // WARN! THIS BELOW IS NOT POSIX
    if (t->pgid == t->pid) {
        for (pid_t cpid = 2; cpid < last_pid + 1; cpid++) {
            struct task *c = pidtable_task(cpid);
            if (c && c->pgid == t->pid)
                c->pgid = 1;
        }
    }

    for (size_t i = 0; i < t->childs_count; i++) {
        pid_t cpid = t->childs[i];
        struct task *c = pidtable_task(cpid);
        if (c && c->parent == t->pid)
            c->parent = 1;
    }

    ienable();
    if (current_task == t) for (;;) suspend();
}

void task_release_mem(struct task *t) {
    if (!t || t == &ktask)
        return;

    for (size_t i = 0; i < t->occupied; i++)
        if (t->sections[i].ptr && t->sections[i].n)
            kfreen((void *) t->sections[i].ptr, t->sections[i].n);

    memset(t->sections, 0, sizeof(t->sections));

    if (!t->is_kthread && t->pagedir) {
        arch_free_pages(t->pagedir);
        t->pagedir = 0;
    }

    kfreen((void *) t->kstack, STACKSIZ);
}

void task_schedule(struct handler_args *args) {
    current_task->sp = (uword_t) args->stack;

    struct task *prev = current_task;
    current_task = find_next_alive();

    if (current_task == prev)
        return;

    task_sig_dispatch(current_task);

    arch_task_update_kstack(current_task->kstack + (STACKSIZ * meminfo.pagesize));

    if (current_task->pagedir != prev->pagedir)
        arch_pagedir_set(current_task->pagedir);

    args->stack = current_task->sp;
}
