# macro to create ISR for 
.macro ISR_NOERR index
    .global isr\index
    isr\index:
        pushl $0
        pushl $\index
        jmp common_isr_handler
.endm

.macro ISR_ERR index
    .global isr\index
    isr\index:
        pushl $\index
        jmp common_isr_handler
.endm
