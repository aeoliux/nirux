#include <irq.h>
#include <ioport.h>
#include <errno.h>
#include <ssb.h>
#include <fs.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <kmem.h>
#include <unistd.h>

static inline size_t veclen(const char **v) {
    if (!v) return 0;

    size_t i = 0;
    while (v[i] != NULL)
        i++;

    return i;
}

int exec_ssb(int src_type, uword_t src, const char **argv, const char **envp, struct handler_args *args) {
    // CHECK FILE PERMISSIONS TODO!

    uint8_t b[sizeof(struct ssb_header) + (SECTION_COUNT - 1) * sizeof(struct ssb_section)];
    memset(b, 0, sizeof(b));

    struct ssb_header *hdr = (struct ssb_header *) b;

    int fd = -1;
    int result;
    uint8_t *buffer;
    switch (src_type) {
        case SSB_SRC_FILE:
            fd = (int) src;
            buffer = b;

            if ((result = lseek(fd, 0, SEEK_SET)) < 0)
                return result;

            result = read(fd, hdr, sizeof(b));
            if (result < (int) sizeof(struct ssb_header))
                return result >= 0 ? -ENOEXEC : result;
            break;
        case SSB_SRC_MEM:
            buffer = (uint8_t *) src;
            hdr = (struct ssb_header *) src;
            break;
    }

    // PERFORM ABI CHECK TODO!

    // printf("Magic: %x, Arch: %x, ABI: %x, Entry: %x, Sections: %x\n", hdr->magic, (uint32_t) hdr->abi, (uint32_t) hdr->abi_flags, (uint32_t) hdr->entry, (uint32_t) hdr->section_count);

    struct ssb_section *sc = (struct ssb_section *) &buffer[sizeof(struct ssb_header)];
    if (hdr->magic != SSB_MAGIC || sc->type != SSB_SECTION_STACK)
        return -ENOEXEC;

    hdr->section_count = (SECTION_COUNT - 1 < hdr->section_count) ? SECTION_COUNT - 1 : hdr->section_count;

    uword_t ustacksiz = ustacksiz = (uword_t) sc[0].length / meminfo.pagesize;
    size_t argc = veclen(argv), envc = veclen(envp);
    size_t envsize = argc + envc;

    struct task *t = current_task;
    if (t == &ktask) {
        t = task_create(0, hdr->entry, ustacksiz);
        if (!t)
            return -ENOMEM;
    }

    size_t oldenvsiz = t->env_section ? t->sections[t->env_section].n : 0;
    uword_t oldenvptr = t->env_section ? t->sections[t->env_section].ptr : 0;
    t->occupied = hdr->section_count;
    if (envsize) {
        t->sections[t->occupied].n = (envsize / meminfo.pagesize) + ((envsize % meminfo.pagesize) > 0);
        t->sections[t->occupied].rw = 0;
        t->sections[t->occupied].ptr = (uword_t) kalloc(t->sections[t->occupied].n);
        if (!t->sections[t->occupied].ptr)
            goto error;

        t->env_section = t->occupied++;

        char *dst = (char *) t->sections[t->env_section].ptr;
        size_t offset = 0;

        for (ssize_t i = envc - 1; i >= 0; i--) {
            size_t n = strlen(envp[i]) + 1;
            memcpy(dst + offset, envp[i], n);
            offset += n;
        }

        for (ssize_t i = argc - 1; i >= 0; i--) {
            size_t n = strlen(argv[i]) + 1;
            memcpy(dst + offset, argv[i], n);
            offset += n;
        }
    }

    if (oldenvptr && oldenvsiz)
        kfreen((void *) oldenvptr, oldenvsiz);

    for (size_t i = 0; i < hdr->section_count; i++) {
        struct ssb_section *s = &sc[i];

        t->sections[i].stack = 0;

        size_t pages = ((uword_t) s->length / meminfo.pagesize) + (((uword_t) s->length % meminfo.pagesize) > 0);

        if (((s->type & SSB_SECTION_LOAD) && !s->length)) {
            if (t->sections[i].ptr)
                kfreen((void *) t->sections[i].ptr, pages);
            t->sections[i].ptr = 0;
            t->sections[i].n = 0;
            continue;
        }

        if (t->sections[i].ptr) {
            if (t->sections[i].n != pages) {
                kfreen((void *) t->sections[i].ptr, pages);
                t->sections[i].ptr = 0;
            }
        }

        if (!t->sections[i].ptr) {
            t->sections[i].ptr = (uword_t) kalloc(pages);
            if (!t->sections[i].ptr)
                goto error;
        }

        if (s->type == SSB_SECTION_STACK) {
            if (t->ustack != t->sections[i].ptr) {
                if (t == current_task) {
                    t->ustack = t->sections[i].ptr;
                    t->ustacksiz = pages;
                } else {
                    kfreen((void *) t->sections[i].ptr, t->sections[i].n);
                    t->sections[i].ptr = t->ustack;
                    t->sections[i].n = t->ustacksiz;
                }
            }

            t->sections[i].stack = 1;
        }

        t->sections[i].n = pages;
        t->sections[i].rw = s->type & SSB_SECTION_WRITE;
        memset((void *) t->sections[i].ptr, 0, pages * meminfo.pagesize);

        if (s->type & SSB_SECTION_LOAD) {
            switch (src_type) {
                case SSB_SRC_FILE:
                    if ((result = lseek(fd, s->offset, SEEK_SET)) < 0)
                        goto error;

                    if ((int64_t) read(fd, (void *) t->sections[i].ptr, s->length) < (int64_t) s->length)
                        goto error;

                    break;
                
                case SSB_SRC_MEM:
                    memcpy((void *) t->sections[i].ptr, buffer + s->offset, s->length);
            }
        }
    }

    if (current_task == t)
        idisable();

    uint8_t *sp = (uint8_t *) t->ustack + (t->ustacksiz * meminfo.pagesize);
    uword_t pagedir = arch_setup_pages(t->sections,  t->occupied);
    if (!pagedir)
        goto error;

    sp -= sizeof(char **);
    *((char **) sp) = NULL;

    if (envsize) {
        size_t offset = 0;

        for (ssize_t i = envc - 1; i >= 0; i--) {
            size_t n = strlen((void *) t->sections[t->env_section].ptr + offset) + 1;
            sp -= sizeof(char **);
            *((char **) sp) = t->sections[t->env_section].vaddr + offset;
            offset += n;
        }

        sp -= sizeof(char **);
        *((char **) sp) = NULL;
        for (ssize_t i = argc - 1; i >= 0; i--) {
            size_t n = strlen((void *) t->sections[t->env_section].ptr + offset) + 1;
            sp -= sizeof(char **);
            *((char **) sp) = t->sections[t->env_section].vaddr + offset;
            offset += n;
        }
    } else {
        sp -= sizeof(char **);
        *((char **) sp) = NULL;
    }

    sp -= sizeof(int);
    *((int *) sp) = argc;

    t->pagedir = pagedir;
    if (current_task == t) {
        close(fd);
        if (task_switch(args, hdr->entry, envsize) < 0)
            goto error;

        return 0;
    } else {
        arch_task_init_stack(t, hdr->entry, envsize);
        t->state = TASK_RUNNING;
    }

    return 0;

error: 
    for (int j = 0; j < hdr->section_count; j++)
        if (t->sections[j].ptr)
            kfreen((void *) t->sections[j].ptr, t->sections[j].n);
    
    if (current_task == t)
        task_exit(t, TASK_FAULT, 0);

    task_release_mem(t);
    return -ENOMEM;
}

/*
struct task *read_ssb(struct task *cur, const char *path) {
    // CHECK FILE PERMISSIONS TODO!

    struct ssb_header hdr;
    if (fs_read_path(path, 0, &hdr, sizeof(hdr)) < (ssize_t) sizeof(hdr) || hdr->magic != SSB_MAGIC)
        return 0;

    // PERFORM ABI CHECK TODO!

    // printf("Magic: %x, Arch: %x, ABI: %x, Entry: %x, Sections: %x\n", hdr->magic, (uint32_t) hdr->abi, (uint32_t) hdr->abi_flags, (uint32_t) hdr->entry, (uint32_t) hdr->section_count);

    struct ssb_section sc[SECTION_COUNT - 2];
    hdr->section_count = (SECTION_COUNT - 2 < hdr->section_count) ? SECTION_COUNT - 2 : hdr->section_count;
    if (
        fs_read_path(
            path, sizeof(hdr), &sc, hdr->section_count * sizeof(struct ssb_section)
        ) < (ssize_t) (hdr->section_count * sizeof(struct ssb_section))
        || sc[0].type != SSB_SECTION_STACK
    )
        return 0;

    uword_t ustacksiz = ustacksiz = (uword_t) sc[0].length / meminfo.pagesize;
            
    struct task *t = NULL;
    if (!ustacksiz || !(t = task_create((current_task->user == ~0) ? 0 : current_task->user, hdr->entry, ustacksiz))) {
        if (t) {
            t->state = TASK_FAULT;
            task_release_mem(t);
        }

        return 0;
    }

    t->occupied = 0;
    for (int i = 0; i < hdr->section_count; i++) {
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
                if ((int64_t) fs_read_path(path, s->offset, (void *) t->sections[i].ptr, s->length) < (int64_t) s->length)
                    goto error;
            }
        }

        t->occupied += t->sections[i].n;
    }

    t->pagedir = arch_setup_pages(t->sections, hdr->section_count);
    if (!t->pagedir)
        goto error;

    return t;

error: 
    for (int j = 0; j < hdr->section_count; j++)
        if (t->sections[j].ptr)
            kfreen((void *) t->sections[j].ptr, t->sections[j].n);

    task_release_mem(t);
    return NULL;
}
*/