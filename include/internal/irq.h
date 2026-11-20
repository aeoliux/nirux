#pragma once

#include <stdint.h>

struct handler_args {
    uword_t int_no, err_code;
    uword_t return_address;
    uword_t arg0, arg1, arg2, arg3, arg4, arg5, arg6;
};

typedef struct {
    uint8_t enabled, does_return;
    uword_t (*handler)(struct handler_args *);
} ihandler_t;

extern ihandler_t *ihandlers;

void register_isr(uint8_t gate, uint32_t isr, uint8_t usercallable);
uint8_t register_irq(uint32_t gate, uword_t (*handler)(struct handler_args *), uint8_t does_return);
uint8_t set_irq_state(uint32_t gate, uint8_t enabled);

uword_t syscall(struct handler_args *args);

void register_syscall();