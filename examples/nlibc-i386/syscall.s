.text
.global syscall

syscall:
    push %ebp
    push %edi
    push %esi
    push %ebx

    movl 44(%esp), %ebp
    movl 40(%esp), %edi
    movl 36(%esp), %esi
    movl 32(%esp), %edx
    movl 28(%esp), %ecx
    movl 24(%esp), %ebx
    movl 20(%esp), %eax

    int $0x80

    pop %ebx
    pop %esi
    pop %edi
    pop %ebp

    ret
