#include <ssb.h>
#include <fs.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <kmem.h>

struct task *read_ssb(struct task *cur, const char *path) {
    // CHECK FILE PERMISSIONS TODO!

    struct ssb_header hdr;
    if (fs_read_path(path, 0, &hdr, sizeof(hdr)) < sizeof(hdr) || hdr.magic != SSB_MAGIC)
        return 0;

    // PERFORM ABI CHECK TODO!

    // printf("Magic: %x, Arch: %x, ABI: %x, Entry: %x, Sections: %x\n", hdr.magic, (uint32_t) hdr.abi, (uint32_t) hdr.abi_flags, (uint32_t) hdr.entry, (uint32_t) hdr.section_count);

    struct ssb_section sc[SECTION_COUNT];
    hdr.section_count = (SECTION_COUNT < hdr.section_count) ? SECTION_COUNT : hdr.section_count;
    if (
        fs_read_path(
            path, sizeof(hdr), &sc, hdr.section_count * sizeof(struct ssb_section)
        ) < hdr.section_count * sizeof(struct ssb_section)
        || sc[0].type != SSB_SECTION_STACK
    )
        return 0;

    uword_t ustacksiz = ustacksiz = (uword_t) sc[0].length / meminfo.pagesize;
            
    struct task *t;
    if (!ustacksiz || !(t = task_create(0, hdr.entry, ustacksiz))) {
        if (t) {
            t->state = TASK_FAULT;
            task_release_mem(t);
        }

        return 0;
    }

    t->occupied = 0;
    for (int i = 0; i < hdr.section_count; i++) {
        struct ssb_section *s = &sc[i];

        if (s->type == SSB_SECTION_STACK) {
            t->sections[i].rw = 1;
            t->sections[i].n = ustacksiz;
            t->sections[i].ptr = t->ustack;
        } else if ((s->type & SSB_SECTION_LOAD && s->length) || (s->type & SSB_SECTION_LOAD) != SSB_SECTION_LOAD){
            // printf("Type: %x, Offset: %x, Length: %x\n", (uint32_t) s->type, (uint32_t) s->offset, (uint32_t) s->length);

            size_t pages = ((uword_t) s->length / meminfo.pagesize) + (((uword_t) s->length % meminfo.pagesize) > 0);

            t->sections[i].ptr = (uword_t) kalloc(pages);
            if (!t->sections[i].ptr) goto error;

            memset((void *) t->sections[i].ptr, 0, pages * meminfo.pagesize);

            t->sections[i].n = pages;
            t->sections[i].rw = s->type & SSB_SECTION_WRITE;

            if (s->type & SSB_SECTION_LOAD) {
                if (fs_read_path(path, s->offset, (void *) t->sections[i].ptr, s->length) < (uword_t) s->length)
                    goto error;
            }
        }

        t->occupied += t->sections[i].n;
    }

    t->pagedir = arch_setup_pages(t->sections, hdr.section_count);
    if (!t->pagedir)
        goto error;

    return t;

error: 
    for (int j = 0; j < hdr.section_count; j++)
        if (t->sections[j].ptr)
            kfreen((void *) t->sections[j].ptr, t->sections[j].n);

    task_release_mem(t);
    return NULL;
}