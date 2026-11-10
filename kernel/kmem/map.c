#include <stdint.h>
#include <kmem.h>
#include <string.h>

struct memory_info meminfo = {0};

void initialise_map(const void *kernel_end, uword_t pagesize) {
    uword_t total = meminfo.total / pagesize;
    uword_t bitmapsize = total / 8;
    
    uint8_t *bitmap = (uint8_t *) kernel_end;
    uint8_t *memstart = bitmap + bitmapsize + (pagesize - (((uword_t) bitmap + bitmapsize) % pagesize));

    memset(bitmap, 0, bitmapsize);

    meminfo.bitmap = bitmap;
    meminfo.bitmapsize = bitmapsize;
    meminfo.pagescount = total;
    meminfo.pagesize = pagesize;
    meminfo.memstart = memstart;
    meminfo.tracker = 0;
}

void mark_as_used(uword_t addr) {
    uword_t page = addr / meminfo.pagesize;
    meminfo.bitmap[page / 8] |= 1 << (page % 8);
}

void mark_as_unused(uword_t addr) {
    uword_t page = addr / meminfo.pagesize;
    meminfo.bitmap[page / 8] &= ~(1 << (page % 8));
}

uword_t get_cell_offset_for_addr(uword_t addr) {
    return addr / meminfo.pagesize;
}

uint8_t get_cell_for_addr(uword_t addr) {
    return meminfo.bitmap[get_cell_offset_for_addr(addr) / 8];
}

uint8_t is_addr_allocated(uword_t addr) {
    uint8_t cell = get_cell_for_addr(addr);

    return (cell & (1 << (get_cell_offset_for_addr(addr) % 8))) != 0;
}

uword_t count_used_pages() {
    uword_t count = 0;

    for (uword_t addr = 0x0; addr < meminfo.total; addr += meminfo.pagesize)
        count += is_addr_allocated(addr);

    return count;
}

uword_t count_used_memory() {
    return count_used_pages() * meminfo.pagesize;
}

void lock_region(uword_t start, uword_t count) {
    // Calculate first cell which should be fully locked (first mid cell)
    uword_t mid_start = start % meminfo.pagesize ? start - (start % meminfo.pagesize) + meminfo.pagesize : start;
    
    // Calculate how many cells are there after first mid cell
    uword_t count_after_mid = count - (mid_start - start);

    // Calculate how many mid cells are there
    uword_t mid_count = count_after_mid / meminfo.pagesize;

    // Calculate remainders on the edges of mid cells
    uword_t before_mid_remainder = count - count_after_mid;

    // Lock page before
    if (before_mid_remainder)
        mark_as_used(start);

    // If first mid address is not the same as start address and there are mid cells
    if (mid_count >= 8) {
        // Calculate first mid cell bitmap offset
        uword_t mid_start_offset = mid_start / meminfo.pagesize / 8;

        // Calculate how many bytes are assigned to mid cells
        uword_t mid_bytes_count = mid_count / 8;

        // Memset bitmap for all mid cells to be 0xff
        memset(meminfo.bitmap + mid_start_offset, 0xff, mid_bytes_count);
    }

    // Check how many mid pages were not disabled (ignored by division by 8 and suffixes)
    uword_t processed_mid_pages = (mid_count / 8) * 8;
    uword_t remaining = mid_start + (processed_mid_pages * meminfo.pagesize);

    // Mark remaining pages
    while (remaining < start + count) {
        mark_as_used(remaining);
        remaining += meminfo.pagesize;
    }
}