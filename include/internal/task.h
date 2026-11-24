#pragma once

#include <fs.h>
#include <regs.h>
#include <irq.h>
#include <signal.h>

#define STACKSIZ 2

struct file_descriptor {
    size_t tasks;

    int flags;
    char path[MAXPATHSIZ + 1];

    struct fs *mnt;
    struct fs_direntry ref;
};

enum task_section_type {
    SECTION_TEXT,
    SECTION_RODATA,
    SECTION_DATA,
    SECTION_BSS,
    SECTION_STACK,

    SECTION_ENV,

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
    uint8_t stack;
    size_t n; // SIZE IN PAGES

    void *vaddr;
    uword_t ptr; // PAGE ALIGNED
} __attribute__((__packed__));

#define MAXFDS ((4096 - 121 - (7 * sizeof(uword_t))) / (int) sizeof(uint64_t) / 2)
#define MAX_CHILDS ((8192 - 125 - (38 * sizeof(uword_t)) - (MAXFDS * 2 * sizeof(uint64_t))) / sizeof(pid_t))

struct task {
    pid_t pid;
    pid_t parent;
    pid_t sid, pgid;
    uint16_t childs_count;

    uint16_t state;
    uint64_t substate;

    uword_t is_kthread;
    uword_t pagedir;

    uint8_t env_section;
    uword_t ustack;
    uword_t ustacksiz;
    uword_t kstack;
    uword_t sp;
    uword_t occupied;

    uint32_t user, euser;

    struct task_section sections[SECTION_COUNT];

    uint64_t fds[MAXFDS * 2];

    uint32_t sig_pending;
    sighandler_t sig_handlers[SIGCOUNT];

    pid_t childs[MAX_CHILDS];
} __attribute__((__aligned__(8192)));

extern struct task ktask;
extern struct task *current_task;
extern struct task *last_task;
extern struct task **pidtab;
extern size_t max_pids;
extern pid_t last_pid;

int tasking_init();
struct task *task_create(uword_t flags, uword_t entry, uword_t ustacksiz);
void task_schedule(struct handler_args *args);
void task_msleep(struct task *t, uword_t ms);
void task_release_mem(struct task *t);
void task_sig_default(struct task *t, int sig);
void task_sig_dispatch(struct task *t);
void task_exit(struct task *t, uint16_t state, uint64_t substate);

pid_t task_clone(struct task *t);
int task_switch(struct handler_args *args, uword_t entry, size_t envsize);

int task_sig_send(struct task *t, int sig);
int task_sig_send_self(int sig);

void pidtable_init();
pid_t pidtable_register(struct task *t);
struct task *pidtable_task(pid_t pid);

// ARCH DEPENDENT

void arch_setup_ktask();
void arch_task_update_kstack(uword_t ksp);
void arch_pagedir_set(uword_t pagedir);
uword_t arch_setup_pages(struct task_section *sections, size_t count);
void arch_task_init_stack(struct task *t, uword_t entry, size_t envsize);
void arch_task_prepare_copied_stack(struct task *t, uword_t pid);
uword_t arch_regs_get_sp(struct regs *r);
void arch_regs_set_sp(struct regs *r, uword_t sp);
void arch_free_pages(uword_t pd);

void idisable();
void ienable();