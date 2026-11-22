#include <kmem.h>
#include <panic.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <i8259.h>

uint32_t page_directory[1024] __attribute__((__aligned__(4096)));
uint32_t *page_tables[1024];
uint32_t page_tables_count = 0;

void arch_pagedir_set(uword_t pagedir) {
    __asm__ volatile (
        "cli\n"
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        "sti"
        :
        : "r"(pagedir)
        : "eax", "memory"
    );
}

void arch_paging_init() {
    uword_t *ret_addr = (uword_t *) __builtin_return_address(0);

    page_tables_count = meminfo.total / (4 * 1024 * 1024);
    if (meminfo.total % (4 * 1024 * 1024))
        page_tables_count++;

    if (page_tables_count > 1024)
        page_tables_count = 1024;

    memset(page_directory, 0, 4096);

    for (size_t t = 0; t < page_tables_count; t++) {
        if (!(page_tables[t] = kalloc(1)))
            panic("failed to allocate buffer for page no. %u\n", ret_addr, t + 1);

        for (size_t p = 0; p < 1024; p++) {
            uint32_t addr = (t * 1024 + p) *4096;
            page_tables[t][p] = addr | 0x3;
        }

        page_directory[t] = ((uint32_t) page_tables[t]) | 0x3;
    }

    arch_pagedir_set((uword_t) page_directory);
}

void arch_setup_ktask() {
    i8259_mask_irq(32);
    arch_paging_init();
    i8259_unmask_irq(32);

    ktask.pagedir = (uword_t) page_directory;
}

void arch_free_pages(uword_t pd) {
    uint32_t *pagedir = (uint32_t *) pd;

    for (size_t k = 512; k < 1024; k++)
        if (pagedir[k] & 0x1)
            kfree((void *)(pagedir[k] & ~0xfff));

    kfree(pagedir);
}

uword_t arch_setup_pages(struct task_section *sections, size_t count) {
    uint32_t *pagedir = kalloc(1);
    if (!pagedir)
        return 0;

    memset(pagedir, 0, 4096);
    for (size_t i = 0; i < page_tables_count; i++)
        pagedir[i] = page_directory[i];

    uword_t offset = 0x80000000;
    for (size_t i = 0; i < count; i++) {
        struct task_section *s = &sections[i];

        s->vaddr = (void *) offset;
        uint16_t flags = s->rw ? 0x7 : 0x5;

        for (size_t j = 0; j < s->n; j++) {
            size_t dir_off = (offset >> 22) & 0x3ff;
            size_t tab_off = (offset >> 12) & 0x3ff;

            if (!(pagedir[dir_off] & 0x1)) {
                uint32_t *new = kalloc(1);
                if (!new) {
                    arch_free_pages((uword_t) pagedir);
                    return 0;
                }

                memset(new, 0, 4096);
                pagedir[dir_off] = pagedir[dir_off] | 0x7;
            }

            uint32_t *ptr = (uint32_t *)(pagedir[dir_off] & ~0xfff);
            ptr[tab_off] = (uint32_t)((s->ptr + (j * 4096)) | flags);

            offset += 4096;
        }
    }

    return (uword_t) pagedir;
}
