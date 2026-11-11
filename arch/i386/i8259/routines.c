#define __i8259__internal
#include <i8259.h>
#include <ioport.h>

void i8259_initialise() {
    i8259_irq_remap(0x20, 0x28);
}