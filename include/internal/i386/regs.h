#pragma once

#include <stdint.h>

struct regs {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed));

extern struct idt_entry idt[256];

void gdt_load();
void idt_load();
void idt_set_gate(uint8_t n, uint32_t addr, uint8_t flags);

void initialise_basic_isrs();

