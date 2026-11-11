#pragma once

#include <stdint.h>

#if defined (__i386__)
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
#endif

struct handler_args {
    uword_t int_no, err_code;
    uword_t return_address;
    uword_t arg0, arg1, arg2, arg3, arg4, arg5, arg6;
};

typedef struct {
    uint8_t enabled, does_return;
    uword_t (*handler)(struct handler_args *);
} ihandler_t;

extern ihandler_t ihandlers[256];

void register_isr(uint8_t gate, uint32_t isr, uint8_t usercallable);
uint8_t register_irq(uint32_t gate, ihandler_t handler, uint32_t isr, uint8_t usercallable);

uword_t syscall(struct handler_args *args);

void register_syscall();