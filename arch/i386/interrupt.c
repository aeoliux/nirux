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

uword_t double_fault(struct handler_args *args) {
    printf("// Caught double fault! (eip = 0x%x)\n", args->return_address);
    return 0;
}

void register_isr(uint8_t gate, uint32_t isr, uint8_t usercallable) {
    idt_set_gate(gate, isr, usercallable ? 0xee : 0x8e);
}

extern void isr0x0();
extern void isr0x1();
extern void isr0x8();
extern void isr0x20();
extern void isr0x80();
void initialise_basic_isrs() {
    ihandler_t i = { 0, 0, 0 };

    // Div by 0
    register_irq(0x0, i, (uint32_t) isr0x0, 0);

    register_irq(0x1, i, (uint32_t) isr0x1, 0);

    // Timer
    i.enabled = 1;
    i.handler = timer_irq;
    register_irq(0x20, i, (uint32_t) isr0x20, 0);

    // Double fault
    i.handler = double_fault;
    register_irq(0x8, i, (uint32_t) isr0x8, 0);
}