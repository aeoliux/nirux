#define __i8259__internal
#include <i8259.h>
#include <ioport.h>

void i8259_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(PIC2_COMM, PIC_EOI);

    outb(PIC1_COMM, PIC_EOI);
}