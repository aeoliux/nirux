.text
.global _start

_start:
    mov %esp, %ebp

    call main

    mov %eax, %ebx
    mov $1, %eax
    int $0x80
