#pragma once

#include <stdint.h>
#include <irq.h>

#define SSB_MAGIC 0x0A425353 // ascii: SSB\n

enum SSB_ARCH {
    SSB_64          = 0x01,

    SSB_I386        = 0x02,
    SSB_ARM         = 0x04,

    SSB_X64         = SSB_I386 | SSB_64,
    SSB_AARCH64     = SSB_ARM | SSB_64
};

enum SSB_I386_ABI {
    // 32 bit
    SSB_ABI_I386,                                 // generic i386
    SSB_ABI_I486        = 0x1,                    // generic i386 + i486 ext.
    SSB_ABI_I586        = SSB_ABI_I486 | 0x2,     // i486 + i586 ext.
    SSB_ABI_I586MMX     = SSB_ABI_I586 | 0x4,     // i586 + mmx (pentium mmx)
    SSB_ABI_I686        = SSB_ABI_I586MMX | 0x8,  // i586 + i686 ext.
    SSB_ABI_I786        = SSB_ABI_I686 | 0x10,    // i686 + pentium 3 ext. (sse, pae, etc.)

    // 64 bit
    // TODO!
};

enum SSB_SECTION_TYPE {
    SSB_SECTION_WRITE   = 0x1,
    SSB_SECTION_LOAD    = 0x2,

    SSB_SECTION_TEXT    = 0x00 | SSB_SECTION_LOAD,
    SSB_SECTION_RODATA  = 0x10 | SSB_SECTION_LOAD,
    SSB_SECTION_DATA    = 0x20 | SSB_SECTION_WRITE | SSB_SECTION_LOAD,
    SSB_SECTION_BSS     = 0x40 | SSB_SECTION_WRITE,
    SSB_SECTION_STACK   = 0x80 | SSB_SECTION_WRITE,
};

struct ssb_section {
    uint16_t type; //  SSB_SECTION_TYPE
    uint64_t offset;
    uint64_t length;
} __attribute__((__packed__));

struct ssb_header {
    uint32_t magic; // must be SSB_MAGIC
    uint64_t abi;
    uint64_t abi_flags;
    uint64_t entry;
    uint64_t size;

    uint8_t section_count;
    struct ssb_section sections[];
} __attribute__((__packed__));

enum SSB_SOURCE {
    SSB_SRC_MEM,
    SSB_SRC_FILE
};

extern struct ssb_header *ssb_payload;
extern size_t ssb_payload_size;

// int exec_ssb(int fd, const char **argv, const char **envp, struct handler_args *args);
int exec_ssb(int src_type, uword_t src, const char **argv, const char **envp, struct handler_args *args);