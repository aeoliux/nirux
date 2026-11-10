#include <kmem.h>

uword_t find_first_free_pages(uword_t at, uword_t n) {
    uword_t max_start = meminfo.pagescount - n;

    for (; at <= max_start; at++) {
        if (!is_addr_allocated(at * meminfo.pagesize)) {
            uword_t continuation = 1;
            
            for (
                ;!(is_addr_allocated((at + continuation) * meminfo.pagesize)) && continuation < n
                ;continuation++
            );

            if (continuation >= n)
                return at;
        }
    }

    return 0;
}

void *kalloc(uword_t n) {
    if (meminfo.used + (meminfo.pagesize * n) >= meminfo.total)
        return NULL;

    if (is_addr_allocated(meminfo.tracker * meminfo.pagesize) || meminfo.tracker + n >= meminfo.pagescount)
        meminfo.tracker = find_first_free_pages(0x0, n);
    else if (n > 1)
        meminfo.tracker = find_first_free_pages(meminfo.tracker, n);

    if (!meminfo.tracker)
        return NULL;

    uword_t next_possible_page_addr = meminfo.tracker * meminfo.pagesize;
    
    for (uword_t offset = 0; offset < n; offset++)
        mark_as_used(next_possible_page_addr + (offset * meminfo.pagesize));

    meminfo.used += n * meminfo.pagesize;
    meminfo.tracker += n;
    return (void *) next_possible_page_addr;
}

void kfree(void *pageptr) {
    // uword_t eip = (uword_t) __builtin_return_address(0);

    if ((uword_t) pageptr % meminfo.pagesize) {
        __asm__ volatile ("hlt");
        //panic("attempted to kfree() a address not refering to mempage ('%u')", &eip, pageptr);
    }

    mark_as_unused((uword_t) pageptr);

    meminfo.used -= meminfo.pagesize;
}