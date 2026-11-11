# track changes of
TRACK += \
	arch/i386/linkerconfig.ld

# objects to be compiled
KOBJ += \
	arch/i386/boot.o \
	arch/i386/init.o \
	arch/i386/io.o \
	arch/i386/panic.o \
	arch/i386/mem.o \
	arch/i386/irq.o \
	arch/i386/gdt.o \
	arch/i386/interrupt.o

include arch/i386/i8259/Makefile
include arch/i386/pit/Makefile