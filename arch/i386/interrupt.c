#include <irq.h>
#include <pit.h>
#include <i8259.h>
#include <panic.h>
#include <stdio.h>

void idt_set_gate(uint8_t n, uint32_t addr, uint8_t flags) {
    __asm__ volatile ("cli");

    idt[n].offset_low = addr & 0xffff;
    idt[n].offset_high = (addr >> 16) & 0xffff;
    idt[n].selector = 0x8;
    idt[n].zero = 0;
    idt[n].type_attr = flags;

    __asm__ volatile ("sti");
}

void i386_interrupt_handler(struct regs *r) {
    if (r->int_no > 255 || (!ihandlers[r->int_no].enabled && (r->int_no < 32 || r->int_no > 47)))
        panic("illegal interrupt '%x' ('%u')", &r->eip, r->int_no, r->int_no);

    ihandler_t *ihandler = &ihandlers[r->int_no];
    if (ihandler->handler) {
        struct handler_args args = {r->int_no, r->err_code, r->eip, r->eax, r->ebx, r->ecx, r->edx, r->esi, r->edi, r->ebp};
        uword_t result = ihandler->handler(&args);
        if (ihandler->does_return)
            r->eax = result;
    }

    if (r->int_no >= 32 && r->int_no <= 47) {
        i8259_eoi(r->int_no - 0x20);
    }
}

uword_t i386_cpu_exception(struct handler_args *args) {
    printf("// Exception No. %x\n", args->int_no);
    return 0;
}

uword_t double_fault(struct handler_args *args) {
    printf("// Caught double fault! (eip = 0x%x)\n", args->return_address);
    return 0;
}

void register_isr(uint8_t gate, uint32_t isr, uint8_t usercallable) {
    idt_set_gate(gate, isr, usercallable ? 0xee : 0x8e);
}

extern void isr0x0();
extern void isr0x1();
extern void isr0x2();
extern void isr0x3();
extern void isr0x4();
extern void isr0x5();
extern void isr0x6();
extern void isr0x7();
extern void isr0x8();
extern void isr0x9();
extern void isr0xA();
extern void isr0xB();
extern void isr0xC();
extern void isr0xD();
extern void isr0xE();
extern void isr0xF();
extern void isr0x10();
extern void isr0x11();
extern void isr0x12();
extern void isr0x13();
extern void isr0x14();
extern void isr0x15();
extern void isr0x16();
extern void isr0x17();
extern void isr0x18();
extern void isr0x19();
extern void isr0x1A();
extern void isr0x1B();
extern void isr0x1C();
extern void isr0x1D();
extern void isr0x1E();
extern void isr0x1F();
extern void isr0x20();
extern void isr0x21();
extern void isr0x22();
extern void isr0x23();
extern void isr0x24();
extern void isr0x25();
extern void isr0x26();
extern void isr0x27();
extern void isr0x28();
extern void isr0x29();
extern void isr0x2A();
extern void isr0x2B();
extern void isr0x2C();
extern void isr0x2D();
extern void isr0x2E();
extern void isr0x2F();

void (*const i386_isrs[])() = {
    isr0x0, isr0x1, isr0x2, isr0x3,
    isr0x4, isr0x5, isr0x6, isr0x7,
    isr0x8, isr0x9, isr0xA, isr0xB,
    isr0xC, isr0xD, isr0xE, isr0xF,
    isr0x10, isr0x11, isr0x12, isr0x13,
    isr0x14, isr0x15, isr0x16, isr0x17,
    isr0x18, isr0x19, isr0x1A, isr0x1B,
    isr0xC, isr0x1D, isr0x1E, isr0x1F,
    isr0x20, isr0x21, isr0x22, isr0x23,
    isr0x24, isr0x25, isr0x26, isr0x27,
    isr0x28, isr0x29, isr0x2A, isr0x2B,
    isr0x2C, isr0x2D, isr0x2E, isr0x2F
};

ihandler_t i386_ihandlers[256] = {
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // DIV BY ZERO
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // DEBUG
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // NMI
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // BRK POINT
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // OVERFLOW
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // BOUND RANGE
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // INVALID OPCODE
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // DEV NOT AVAI
    { .enabled = 1, .does_return = 0, .handler = double_fault },       // DOUBLE FAULT
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // COPR SEG OVERRUN
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // INVALID TSS
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // SEGMENT NOT PRESENT
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // STACK SEGMENT
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // GENERAL PROTECTION
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // PAGE FAULT
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // RESERVED
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // X87 FP
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // ALIGN CHECK
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // MACHINE CHECK
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // SIMD FP
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // VIRT
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // CONTROL PROTECTION
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // RESERVED
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // -
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // -
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // -
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // -
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // RESERVED
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // HYPERVISOR INJECT
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // VMM COMM
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // SECURITY
    { .enabled = 1, .does_return = 0, .handler = i386_cpu_exception }, // RESERVED
    { .enabled = 1, .does_return = 0, .handler = timer_irq }, // PIT TIMER IRQ
};

void initialise_basic_isrs() {
    ihandlers = i386_ihandlers;

    for (size_t i = 0; i <= 0x30; i++)
        register_isr(i, (uword_t) i386_isrs[i], 0);
}