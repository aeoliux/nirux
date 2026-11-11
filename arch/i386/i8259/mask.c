#define __i8259__internal
#include <i8259.h>
#include <ioport.h>

void i8259_mask_irq(uint8_t irq) {
    uint16_t port;
    if (irq < 8)
        port = PIC1_DATA;
    else {
        port = PIC2_DATA;
        irq -= 8;
    }

    uint8_t value = inb(port) | (1 << irq);
    outb(port, value);
}

void i8259_unmask_irq(uint8_t irq) {
    uint16_t port;
    if (irq < 8)
        port = PIC1_DATA;
    else {
        port = PIC2_DATA;
        irq -= 8;
    }

    uint8_t value = inb(port) & ~(1 << irq);
    outb(port, value);
}