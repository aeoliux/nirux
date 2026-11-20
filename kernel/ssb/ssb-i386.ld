ENTRY(_start)

SECTIONS {
    .header 0 : AT(0) {
        /* SSB_MAGIC */
        LONG(0x0A425353);
        /* SSB_ARCH = i386 */
        QUAD(0x2);
        /* SSB_I386_FLAGS = SSB_ABI_I386 */
        QUAD(0);
        /* entry */
        QUAD(_start);
        /* section count */
        BYTE(5);

        /* sections */
        SHORT(0x81); QUAD(0); QUAD(_stack_end - _stack_start);                      /* .stack */
        SHORT(0x02); QUAD(LOADADDR(.text)); QUAD(_text_end - _text_start);          /* .text */
        SHORT(0x12); QUAD(LOADADDR(.rodata)); QUAD(_rodata_end - _rodata_start);    /* .rodata */
        SHORT(0x23); QUAD(LOADADDR(.data)); QUAD(_data_end - _data_start);          /* .data */
        SHORT(0x41); QUAD(0); QUAD(_bss_end - _bss_start);                          /* .bss */
    }
    _hdr_end = SIZEOF(.header);

    . = 0x80000000;
    .stack ALIGN(4K) (NOLOAD) : { _stack_start = .; . += 8K; _stack_end = .; }

    .text ALIGN(4K) : AT(_hdr_end) {
        _text_start = .; *(.text*) _text_end = .;
    }
    .rodata ALIGN(4K) : AT(LOADADDR(.text) + SIZEOF(.text)) {
        _rodata_start = .; *(.rodata*) _rodata_end = .;
    }
    .data ALIGN(4K) : AT(LOADADDR(.rodata) + SIZEOF(.rodata)) {
        _data_start = .; *(.data*) _data_end = .;
    }
    .bss ALIGN(4K) (NOLOAD) : {
        _bss_start = .; *(.bss* COMMON) _bss_end = .;
    }

    /DISCARD/ : {
        *(.note*)
        *(.comment*)
        *(.eh_frame*)
    }
}