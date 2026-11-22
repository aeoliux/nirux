ENTRY(_start)

SECTIONS {
	. = 1M;

	.text ALIGN(4K) : {
		*(.multiboot)
		*(.text)
	}

	.rodata ALIGN(4K) : {
		*(.rodata)
	}

	.data ALIGN(4K) : {
		*(.data)
	}

	.bss ALIGN(4K) (NOLOAD) : {
		_bss_start = .;
		*(COMMON)
		*(.bss)
		*(.bss.*)
		*(.bssend)
		_bss_end = .;
	}
}