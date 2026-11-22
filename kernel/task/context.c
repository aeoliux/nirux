#include <ioport.h>
#include <kmem.h>
#include <irq.h>
#include <regs.h>
#include <stdio.h>
#include <task.h>
#include <timer.h>
#include <panic.h>

static inline struct task *find_next_alive() {
    static pid_t current_pid = 0;
    uword_t *ip = __builtin_return_address(0);

    struct task *ptr = pidtab[current_pid];
    // struct task *startedWith = current_task;

    for (;; ptr = pidtab[current_pid] ? pidtab[current_pid] : &ktask) {
        current_pid = (current_pid + 1) % (last_pid + 2);
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

    // TODO! DIFFERENT HANDLING FOR TASK_KILLED

    if (state == TASK_FAULT)
        printf("// FAULT: %x\n", (uword_t) substate);

    t->state = state;
    t->substate = substate;

    for (size_t i = 0; i < MAX_CHILDS; i++)
        if (t->childs[i])
            if (pidtab[t->childs[i]]->parent == t->pid)
                task_exit(pidtab[t->childs[i]], TASK_KILLED, 1);

    ienable();
    if (current_task == t) for (;;) suspend();
}

void task_release_mem(struct task *t) {
    if (!t || t == &ktask)
        return;

    for (size_t i = 0; i < SECTION_COUNT; i++)
        if (t->sections[i].ptr && t->sections[i].n)
            kfreen((void *) t->sections[i].ptr, t->sections[i].n);

    if (!t->is_kthread && t->pagedir)
        arch_free_pages(t->pagedir);

    kfreen((void *) t->kstack, STACKSIZ);
}

void task_schedule(struct handler_args *args) {
    current_task->sp = (uword_t) args->stack;

    struct task *prev = current_task;
    current_task = find_next_alive();

    if (current_task == prev)
        return;

    arch_task_update_kstack(current_task->kstack + (STACKSIZ * meminfo.pagesize));

    if (current_task->pagedir != prev->pagedir)
        arch_pagedir_set(current_task->pagedir);

    args->stack = current_task->sp;
}
