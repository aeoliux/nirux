.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO 
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.global KERNEL_START
KERNEL_START:
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bss
.align 16

.global stack_bottom
stack_bottom:
.skip 16384 # 16 KiB
.global stack_top
stack_top:

.section .text
.global _start
.type _start, @function
_start:
	cli

	mov $stack_top, %esp
	mov %esp, %ebp

	push %ebx
	push %eax
	call _i386_init
	add $0x8, %esp

	push $no_task_panic
	call panic
	nop
	_i386_as_halt: 	hlt
					jmp _i386_as_halt

.section .data
no_task_panic:
	.asciz "no payload (kmain() finished)"

.section .bssend
.global KERNEL_END
KERNEL_END:
