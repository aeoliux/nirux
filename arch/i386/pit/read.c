#include <pit.h>
#include <ioport.h>
#include <stdint.h>

uint16_t pit_read_counter() {
    uint16_t counter = 0;

    outb(0x43, 0x0);

    counter = inb(0x40);
    counter |= inb(0x40) << 8;

    return counter;
}