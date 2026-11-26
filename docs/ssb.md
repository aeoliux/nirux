# Statically Sectioned Binary (Format)

Static binary format as simple as possible providing basic and sufficient functionality.

## Important rules
- Binary is always loaded into virtual address of 0x80000000 and all symbols must refer to this address.
- First defined section must always be a stack section (defines stack size) and other sections must be provided right after the stack.

## Header
```c
struct ssb_header {
    uint32_t magic; // must be SSB_MAGIC
    uint64_t abi;
    uint64_t abi_flags;
    uint64_t entry;
    uint64_t size;

    uint8_t section_count;
} __attribute__((__packed__));
```

### Entries
- `magic` = `0x0A425353` (ascii reversed: `SSB\n`) contains signature indicating the data is a proper SSB binary
- `abi` - defines binary architecture - bit 0: is 64 bit, bit 1-63: architecture (ex. i386 - bit 1, arm - bit 2)
- `abi_flags` - specific information about architecture version (ex. i386, i486, i586, mmx, etc.)
- `entry` - virtual address of first instruction
- `size` - file size
- `section_count` - number of sections defined by `struct ssb_section` right after header

## Sections
Each section is defined after each other. First section info is located right after header. Header contains number of sections defined in binary. First section must always be stack info.

### Format
```c
struct ssb_section {
    uint16_t type; //  SSB_SECTION_TYPE
    uint64_t offset;
    uint64_t length;
} __attribute__((__packed__));
```

### Entries
- `type` - section type - `0x81` for stack, other sections can be described as `0x1` (writeable, ex. .data, .stack, .bss) or `0x2` (load from file, ex. .text, .data, .rodata).
- `offset` - offset in image (0 for non-loadable sections).
- `length` - length in image (for loadable) or length of buffer to allocate (for non-loadable)

### Example stack section
This allocates 8KB stack (`0x80000000` - `0x80002000`). Stack section must always be defined first.
```c
struct ssb_section s = {
    .type = 0x81,
    .offset = 0,
    .length = 8192
};
```
