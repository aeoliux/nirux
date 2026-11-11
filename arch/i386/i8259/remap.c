#include <ioport.h>
#define __i8259__internal
#include <i8259.h>

void i8259_irq_remap(uint8_t offset1, uint8_t offset2) {
    // send initialisation command to both PIC with ICW4 (0x10 | 0x1 = 0x11)
    outb(PIC1_COMM, 0x11);
    iowait();
    outb(PIC2_COMM, 0x11);
    iowait();

    // send vector offsets
    outb(PIC1_DATA, offset1);
    iowait();
    outb(PIC2_DATA, offset2);
    iowait();

    // map second pic as slave pic to first pic
    outb(PIC1_DATA, 0x4);
    iowait();
    outb(PIC2_DATA, 0x2);
    iowait();

    // use 8086 mode instead of 8080 mode
    outb(PIC1_DATA, 0x1);
    iowait();
    outb(PIC2_DATA, 0x1);
    iowait();

    // unmask both pics
    outb(PIC1_DATA, 0x0);
    outb(PIC2_DATA, 0x0);
}