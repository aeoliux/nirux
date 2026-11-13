#include <x86floppy.h>
#include <irq.h>

uint8_t x86floppy_irqed = 0;

uword_t x86floppy_irq6(struct handler_args *args) {
    //puts("// x86floppy received IRQ6\n");

    x86floppy_flush_buffer();

    x86floppy_irqed = 1;
    return 0;
}
