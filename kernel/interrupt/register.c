#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <irq.h>

ihandler_t ihandlers[256] = {0};

extern void isr0x80();
uint8_t register_irq(uint32_t gate, ihandler_t handler, uint32_t isr, uint8_t usercallable) {
    if (ihandlers[gate].handler && ihandlers[gate].enabled) {
        puts("// denied access to reassign IRQ\n");
        return -1;
    }

    memcpy(&ihandlers[gate], &handler, sizeof (ihandler_t));

    register_isr(gate, isr, usercallable);

    return 0;
}

void register_syscall() {
    ihandler_t i = { 1, 1, syscall };
    register_irq(0x80, i, (uint32_t) isr0x80, 1);
}