#pragma once

#include <fs.h>
#include <fd.h>
#include <regs.h>
#include <irq.h>

#define STACKSIZ 2

enum task_section_type {
    SECTION_TEXT,
    SECTION_RODATA,
    SECTION_DATA,
    SECTION_BSS,
    SECTION_STACK,

    SECTION_COUNT
};

enum task_state {
    TASK_READY      = 0x0001,

    TASK_RUNNING    = 0x0f00,
    TASK_PAUSED     = TASK_RUNNING | 0x0001,
    TASK_SLEEP      = TASK_RUNNING | 0x0002,    // substate = ms timestamp of wake-up

    TASK_FINISHED   = 0xf000,
    TASK_FAULT      = TASK_FINISHED | 0x0001,   // substate = exception number
    TASK_EXITED     = TASK_FINISHED | 0x0002,   // substate = return code
    TASK_KILLED     = TASK_FINISHED | 0x0004    // substate = signal code
};

struct task_section {
    uint8_t rw;
    size_t n; // SIZE IN PAGES

    uword_t ptr; // PAGE ALIGNED
} __attribute__((__packed__));

struct task {
    uint16_t state;
    uint64_t substate;

    uword_t is_kthread;
    uword_t pagedir;

    uword_t ustack;
    uword_t ustacksiz;
    uword_t kstack;
    uword_t sp;
    uword_t occupied;

    uint32_t user;

    struct task_section sections[SECTION_COUNT];

    struct task *next;

    int fds[];
} __attribute__((__aligned__(4096)));

extern struct task ktask;
extern struct task *current_task;
extern struct task *last_task;

int tasking_init();
struct task *task_create(uword_t flags, uword_t entry, uword_t ustacksiz);
void task_schedule(struct handler_args *args);
void task_msleep(struct task *t, uword_t ms);

void task_release_mem(struct task *t);
void task_exit(struct task *t, uint16_t state, uint64_t substate);

// ARCH DEPENDENT

void arch_setup_ktask();
void arch_task_update_kstack(uword_t ksp);
void arch_pagedir_set(uword_t pagedir);
uword_t arch_setup_pages(struct task_section *sections, size_t count);
void arch_task_init_stack(struct task *t, uword_t entry);
uword_t arch_regs_get_sp(struct regs *r);
void arch_regs_set_sp(struct regs *r, uword_t sp);
void arch_free_pages(uword_t pd);
