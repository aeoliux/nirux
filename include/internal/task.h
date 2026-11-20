#pragma once

#include <fs.h>
#include <fd.h>
#include <regs.h>

#define STACKSIZ 2

enum task_section_type {
    SECTION_TEXT,
    SECTION_RODATA,
    SECTION_DATA,
    SECTION_BSS,

    SECTION_COUNT
};

struct task_section {
    uint8_t type;
    uint8_t rw;
    size_t n; // SIZE IN PAGES

    uword_t ptr; // PAGE ALIGNED
} __attribute__((__packed__));

struct task {
    uword_t is_kthread;
    uword_t pagedir;
    uword_t kstack;
    uword_t sp;
    size_t kstack_size;

    uint16_t uid, gid;

    struct task_section sections[SECTION_COUNT];

    struct task *next;

    int fds[1004];
} __attribute__((__packed__));

extern struct task ktask;
extern struct task *current_task;
extern struct task *last_task;

int tasking_init();
int task_create(uword_t flags, uword_t entry, uword_t pagedir);
struct regs *task_schedule(struct regs *r);

// ARCH DEPENDENT

void arch_setup_ktask();
void arch_task_update_kstack(uword_t ksp);
void arch_pagedir_set(uword_t pagedir);
uword_t arch_setup_pages(struct task_section *sections, size_t count);
void arch_task_init_stack(struct task *t, uword_t entry, uword_t user_sp);
uword_t arch_regs_get_sp(struct regs *r);
void arch_regs_set_sp(struct regs *r, uword_t sp);