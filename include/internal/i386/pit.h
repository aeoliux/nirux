#pragma once

#include <stdint.h>
#include <irq.h>

void pit_set_timer(uint16_t divisor);
void pit_set_frequency(uint32_t hz);
uword_t timer_irq(struct handler_args *args);
uint16_t pit_read_counter();
void pit_set_counter(uint16_t counter);

extern uint16_t timer_ms, timer_ms_f;