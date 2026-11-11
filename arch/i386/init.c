#include <i8259.h>
#include <kmem.h>
#include <multiboot.h>
#include <gdt.h>
#include <irq.h>
#include <pit.h>

extern void kmain();

void _i386_init(uint32_t _, multiboot_info_t *minfo) {
    gdt_load();
    idt_load();

    initialise_basic_isrs();

    i8259_initialise();
    pit_set_frequency(100);

    if (initialise_mem_by_multiboot(minfo)) return;

    kmain();
}