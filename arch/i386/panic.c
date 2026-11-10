#include <stdint.h>
#include <stdio.h>
#include <uname.h>

#if defined (__i386__)
struct regs {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t eip;
};

void _print_registers(struct regs *r) {
    printf("\n/*\n!kernel panic\nds = %x\nedi = %x, esi = %x, ebp = %x, esp = %x\n", r->ds, r->edi, r->esi, r->ebp, r->esp);
    printf("ebx = %x, edx = %x, ecx = %x, eax = %x\n", r->ebx, r->edx, r->ecx, r->eax);

    printf("caller info: return eip = %x\n", r->eip);
}
#endif

void panic(const char *fmt, uint32_t *eip, ...) {
    __asm__ volatile ("cli");

    void *caller = eip ? (void *) *eip : __builtin_return_address(0);

    #if defined (__i386__)
    __asm__ volatile ("push %0" : : "r"(caller));
    __asm__ volatile ("pusha");
    __asm__ volatile ("push %ds");
    __asm__ volatile ("push %esp");
    __asm__ volatile ("call _print_registers");
    __asm__ volatile ("add $44, %esp");
    #endif

    uint8_t *arg = (uint8_t *)&eip + (uword_t) sizeof(eip);
    printf("!%s-%s: ", UNAME_M, UNAME_R);
    vprintf(fmt, arg);
    putchar('\n');
    puts("*/\n");

    while (1) { __asm__ volatile ("hlt"); }
}