#pragma once

#ifdef __i8259__internal
#define PIC1 0x20
#define PIC2 0xa0
#define PIC1_COMM PIC1
#define PIC2_COMM PIC2
#define PIC1_DATA (PIC1 + 1)
#define PIC2_DATA (PIC2 + 1)

#define PIC_EOI 0x20

#endif

#include <stdint.h>

void i8259_initialise();
void i8259_irq_remap(uint8_t offset1, uint8_t offset2);
void i8259_eoi(uint8_t irq);
void i8259_mask_irq(uint8_t irq);
void i8259_unmask_irq(uint8_t irq);