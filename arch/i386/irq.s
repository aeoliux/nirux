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
ISR_ERR 0x8 # double fault

# Timer IRQ
ISR_NOERR 0x20

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
