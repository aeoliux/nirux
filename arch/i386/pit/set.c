#include <ioport.h>
#include <timer.h>
#include <pit.h>

uint32_t timer_freq = 0;
uint16_t timer_ms = 0, timer_ms_f = 0;

void pit_set_timer(uint16_t divisor) {
    uint32_t ms_with_f = divisor * 100000 / 1193182;
    uint16_t ms = ms_with_f / 100;
    uint16_t ms_2digits_remainder = ms_with_f % 100;

    uint32_t freq = 1193182 / divisor;

    outb(0x43, 0x34);
    outb(0x40, (uint8_t) (divisor & 0xff));
    outb(0x40, (uint8_t) ((divisor >> 8) & 0xff));

    timer_freq = freq;
    timer_ms = ms;
    timer_ms_f = ms_2digits_remainder;
}

void pit_set_frequency(uint32_t hz) {
    if (hz <= 18)
        return pit_set_timer(0);
    else if (hz >= 1193181)
        return pit_set_timer(1);

    uint32_t divisor = 1193182 / hz + ((1193182 % hz) >= (1193182 / 2) ? 1 : 0);

    pit_set_timer(divisor);
}

void pit_set_counter(uint16_t counter) {
    outb(0x43, 0x36);

    outb(0x40, counter & 0xff);
    outb(0x40, (counter >> 8) & 0xff);
}