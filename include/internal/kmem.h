#pragma once

#include <stdint.h>

struct memory_info {
    uword_t total;
    uword_t reserved;
    uword_t used;

    uint8_t *bitmap;
    uword_t bitmapsize;
    uword_t pagescount;
    uword_t pagesize;
    uint8_t *memstart;
    uword_t tracker;
} __attribute__((packed));

extern struct memory_info meminfo;

void initialise_map(const void *kernel_end, uword_t pagesize);
void lock_region(uword_t start, uword_t count);

void mark_as_unused(uword_t addr);
void mark_as_used(uword_t addr);

uint8_t get_cell_for_addr(uword_t addr);
uint8_t is_addr_allocated(uword_t addr);
uword_t get_cell_offset_for_addr(uword_t addr);
uword_t count_used_pages();
uword_t count_used_memory();

void *kalloc(uword_t n);
void kfree(void *pageptr);
