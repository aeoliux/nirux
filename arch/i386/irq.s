.include "arch/i386/macros.s"

.section .data
.align 4

.global idt
idt:
.skip 256 * 8 # 256 8-byte long IDT entries
idt_end:

idt_ptr:
    .word idt_end - idt - 1 # always length is entries * 8 - 1
    .long idt

.section .text
.global idt_load
idt_load:
    cli
    lidt (idt_ptr)
    sti

    ret

# SYSCALL interrupt
ISR_NOERR 0x80

# Exceptions
ISR_NOERR 0x0 # div by 0
ISR_NOERR 0x1
ISR_NOERR 0x2
ISR_NOERR 0x3
ISR_NOERR 0x4
ISR_NOERR 0x5
ISR_NOERR 0x6
ISR_NOERR 0x7
ISR_ERR 0x8 # double fault
ISR_NOERR 0x9
ISR_ERR 0xA
ISR_ERR 0xB
ISR_ERR 0xC
ISR_ERR 0xD
ISR_ERR 0xE
ISR_NOERR 0xF
ISR_NOERR 0x10
ISR_ERR 0x11
ISR_NOERR 0x12
ISR_NOERR 0x13
ISR_NOERR 0x14
ISR_NOERR 0x15
ISR_NOERR 0x16
ISR_NOERR 0x17
ISR_NOERR 0x18
ISR_NOERR 0x19
ISR_NOERR 0x1A
ISR_NOERR 0x1B
ISR_NOERR 0x1C
ISR_NOERR 0x1D
ISR_ERR 0x1E
ISR_NOERR 0x1F

# Timer IRQ
ISR_NOERR 0x20
ISR_NOERR 0x21
ISR_NOERR 0x22
ISR_NOERR 0x23
ISR_NOERR 0x24
ISR_NOERR 0x25
ISR_NOERR 0x26
ISR_NOERR 0x27
ISR_NOERR 0x28
ISR_NOERR 0x29
ISR_NOERR 0x2A
ISR_NOERR 0x2B
ISR_NOERR 0x2C
ISR_NOERR 0x2D
ISR_NOERR 0x2E
ISR_NOERR 0x2F

.global common_isr_handler
common_isr_handler:
    # save general registers
    pusha

    # save data segment register
    mov %ds, %ax
    pushl %eax

    # load kernel data segment
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # pass pointer to pushed registers as argument to interrupt handler
    pushl %esp
    call i386_interrupt_handler
    
    # clean up pushed ESP
    add $4, %esp

    # restore segments
    popl %eax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # restore general registers
    popa
    add $8, %esp
    iret
