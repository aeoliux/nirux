#include <stdint.h>
#include <irq.h>
#include <pit.h>
#include <timer.h>
#include <task.h>
#include <i8259.h>

uint32_t timer_elapsed = 0;
uint16_t timer_elapsed_f = 0;

uword_t timer_irq(struct handler_args *args) {
    timer_elapsed += timer_ms;
    timer_elapsed_f += timer_ms_f;
    if (timer_elapsed_f & 100) {
        timer_elapsed_f = 0;
        timer_elapsed++;
    }
    
    i8259_eoi(0);

    struct regs *r = (struct regs *) args->stack;

    task_schedule(args);

    return 0;
}
