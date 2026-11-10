#include <kmem.h>
#include <multiboot.h>

extern const void KERNEL_START, KERNEL_END;
extern void kmain();

int initialise_mem_by_multiboot(multiboot_info_t *info) {
    if (!(info->flags >> 6 & 0x1))
        return 1;

    // Calculate available, total and used memory (mainly by kernel)
    uword_t total = 0;

    for (uword_t i = 0; i < info->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *) (info->mmap_addr + i);

        // printf("[mmsrv] entry: start = %x | length = %x | size = %x | type = %d\n", mmap->addr_low, mmap->len_low, mmap->size, mmap->type);

        total += mmap->len_low;
    }

    meminfo.total = total;

    // Initialise allocators bitmap (default pagesize 4K)
    initialise_map(&KERNEL_END, 4096);

    // Lock region for IVT and bios data
    // It is recommended to disable first page no matter the architecture
    lock_region(0x0, 0x1000);

    // Lock BIOS
    lock_region(0x9c680, 0x100000 - 0x9c680);

    // Lock all regions marked as unavailable
    for (uword_t i = 0; i < info->mmap_length; i += sizeof(multiboot_memory_map_t)) {
        multiboot_memory_map_t *mmap = (multiboot_memory_map_t *) (info->mmap_addr + i);

        if (mmap->type != MULTIBOOT_MEMORY_AVAILABLE && mmap->addr_low < total) {
            lock_region(mmap->addr_low, mmap->len_low);
            
            meminfo.reserved += mmap->len_low;
        }
    }

    // Lock already used regions (kernel + stack + bitmap)
    lock_region((uword_t) &KERNEL_START, (&KERNEL_END - &KERNEL_START) + meminfo.bitmapsize);

    meminfo.used = count_used_memory();
    meminfo.reserved = meminfo.used;

    return 0;
}

void _i386_init(uint32_t _, multiboot_info_t *minfo) {
    if (initialise_mem_by_multiboot(minfo)) return;

    kmain();
}