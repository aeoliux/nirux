#include <i8259.h>
#include <kmem.h>
#include <multiboot.h>
#include <gdt.h>
#include <irq.h>
#include <pit.h>
#include <regs.h>
#include <ssb.h>
#include <string.h>

extern void kmain();

void _i386_init(uint32_t _, multiboot_info_t *minfo) {
    // gdt_load();
    idt_load();

    initialise_basic_isrs();

    i8259_initialise();

    pit_set_frequency(100);

    if (initialise_mem_by_multiboot(minfo)) return;

    if (minfo->flags & (1 << 3) && minfo->mods_count) {
        struct multiboot_mod_list *mods = (struct multiboot_mod_list *) minfo->mods_addr;

        ssb_payload_size = mods[0].mod_end - mods[0].mod_start;
        ssb_payload_size = ((ssb_payload_size % meminfo.pagesize) > 0) + (ssb_payload_size / meminfo.pagesize);
        ssb_payload = kalloc(ssb_payload_size);
        if (ssb_payload) {
            memcpy(ssb_payload, (void *) mods[0].mod_start, mods[0].mod_end - mods[0].mod_start);
        } else {
            ssb_payload_size = 0;
        }
    }

    kmain();
}