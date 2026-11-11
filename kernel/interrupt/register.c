#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <irq.h>

ihandler_t *ihandlers = NULL;

uint8_t register_irq(uint32_t gate, uword_t (*handler)(struct handler_args *), uint8_t does_return) {
    if (ihandlers[gate].handler && ihandlers[gate].enabled) {
        puts("// denied access to reassign IRQ\n");
        return -1;
    }

    memset(&ihandlers[gate], 0, sizeof(ihandler_t));

    ihandlers[gate].handler = handler;
    ihandlers[gate].does_return = does_return;

    return 0;
}

uint8_t set_irq_state(uint32_t gate, uint8_t enabled) {
    if (!ihandlers[gate].handler)
        return 0;

    ihandlers[gate].enabled = enabled;

    return enabled;
}

extern void isr0x80();
void register_syscall() {
    register_irq(0x80, syscall, 1);
    set_irq_state(0x80, 1);
}