#include <regs.h>
#include <task.h>

struct regs *task_schedule(struct regs *r) {
    current_task->sp = (uword_t) r;

    struct task *prev = current_task;
    current_task = current_task->next ? current_task->next : &ktask;

    arch_task_update_kstack(current_task->kstack + current_task->kstack_size);

    if (current_task->pagedir != prev->pagedir)
        arch_pagedir_set(current_task->pagedir);

    return (struct regs *) current_task->sp;
}