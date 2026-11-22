.text
.global _start

_start:
    mov %esp, %ebp

    mov (%ebp), %eax
    mov %ebp, %ebx
    add $4, %ebx
    push %ebx
    push %eax

    inc %eax
    mov $4, %edx
    mul %edx
    add %eax, %ebx
    mov $environ, %edx
    mov (%edx), %ebx

    xor %eax, %eax
    xor %ebx, %ebx
    xor %edx, %edx

    call main
    add $8, %esp

    mov %eax, %ebx
    mov $1, %eax
    int $0x80

.bss
.align 4
.global environ
environ: .zero 4
