#include <stdint.h>
#include <string.h>
#include <task.h>
#include <regs.h>
#include <kmem.h>

extern uint8_t gdt_tss_entry[8];
extern void gdt_load(void);
extern void tss_flush(void);

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1, ss1, esp2, ss2;
    uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs, ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((__packed__));

static struct tss_entry tss;

void arch_task_init_stack(struct task *t, uword_t entry, size_t envsize) {
    struct regs *r = (struct regs *)(t->kstack + (STACKSIZ * meminfo.pagesize) - sizeof(struct regs));
    memset(r, 0, sizeof(struct regs));

    r->ds = t->is_kthread ? 0x10 : 0x23;

    r->eip = entry;
    r->cs = t->is_kthread ? 0x8 : 0x1b;
    r->eflags = 0x202;

    if (!t->is_kthread) {
        r->useresp = 0x80000000 + (t->ustacksiz * meminfo.pagesize) - (1 * sizeof(int)) - ((envsize + 2) * sizeof(char *));
        r->ss = 0x23;
    }

    r->esp = (uword_t) r;
    t->sp = (uword_t) r;
}

void arch_task_prepare_copied_stack(struct task *t, uword_t pid) {
    struct regs *r = (struct regs *)(t->kstack + (STACKSIZ * meminfo.pagesize) - sizeof(struct regs));
    r->eax = pid;

    t->sp = (uword_t) r;
}

void arch_task_update_kstack(uword_t ksp) {
    tss.esp0 = ksp;
}

uword_t arch_regs_get_sp(struct regs *r) {
    return r->esp;
}

void arch_regs_set_sp(struct regs *r, uword_t sp) {
    r->esp = sp;
}

void tss_set_entry(uint32_t base, uint32_t limit) {
    gdt_tss_entry[0] = limit & 0xFF;
    gdt_tss_entry[1] = (limit >> 8) & 0xFF;
    gdt_tss_entry[2] = base & 0xFF;
    gdt_tss_entry[3] = (base >> 8) & 0xFF;
    gdt_tss_entry[4] = (base >> 16) & 0xFF;
    gdt_tss_entry[5] = 0x89;
    gdt_tss_entry[6] = (limit >> 16) & 0x0F;
    gdt_tss_entry[7] = (base >> 24) & 0xFF;
}

void tss_init(uint32_t kernel_esp) {
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = 0x10;
    tss.esp0 = kernel_esp;
    tss.iomap_base = sizeof(tss);

    tss_set_entry((uint32_t)&tss, sizeof(tss) - 1);
    gdt_load();
    tss_flush();
}