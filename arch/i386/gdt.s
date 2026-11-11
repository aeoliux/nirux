.section .data
.align 4

gdt_start:
    # 1. Null descriptor
    .long 0
    .long 0

    # 2. Kernel code segment
    .word 0xffff # limit low 16 bits
    .word 0x00 # base low 16 bits
    .byte 0x00 # base mid 16 bits
    .byte 0x9a # access (code, readable, ring 0)
    .byte 0xcf # flags & limit (high 4 bits)
    .byte 0x00 # base (high 4 bits)

    # 3. Kernel data segment
    .word 0xffff
    .word 0x00
    .byte 0x00
    .byte 0x92 # access (data, writable, ring 0)
    .byte 0xcf
    .byte 0x00
gdt_end:

gdt_ptr:
    .word gdt_end - gdt_start - 1 # size
    .long gdt_start # address

.section .text
.global gdt_load
gdt_load:
    lgdt (gdt_ptr)

    # reload data segments
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # far jump to reload code segment
    ljmp $0x8, $.reload_cs

.reload_cs:
    ret
