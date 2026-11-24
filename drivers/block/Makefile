KOBJ += \
	drivers/block/interface.o \
	drivers/block/operations.o \
	drivers/block/floppy.o

ifeq ($(ARCH), i386)
include drivers/block/x86floppy/Makefile
include drivers/block/isaata/Makefile
endif