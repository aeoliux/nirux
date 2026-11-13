ARCH := i386
MARCH := i486
QEMU_CPU := 486
OPTIMISE := 0

KSTRING := 1.0.0-moonshine

CROSS_COMPILE :=

AS = ${CROSS_COMPILE}as
CC = ${CROSS_COMPILE}cc
AR = ${CROSS_COMPILE}ar
STRIP = ${CROSS_COMPILE}strip

INCLUDE = -Iinclude/libc -Iinclude/internal -Iinclude/internal/$(ARCH)
CFLAGS = -march=$(MARCH) -std=c99 -nostdlib -mno-sse -mno-mmx -msoft-float -nostdinc -nodefaultlibs -fno-pie -fno-pic -ffreestanding -O$(OPTIMISE) -Wall -Wextra -Wno-unused-parameter $(INCLUDE)
LDFLAGS = -march=$(MARCH) -fno-pie -fno-pic -static -ffreestanding -O$(OPTIMISE) -nostdlib -nostdinc -nodefaultlibs -z noexecstack -T arch/$(ARCH)/linkerconfig.ld
ASFLAGS =

ifeq ($(ARCH), i386)
CFLAGS += -m32
ASFLAGS += --32
LDFLAGS += -m32
endif

KOBJ = .kinfo.o
TRACK =

include arch/Makefile
include kernel/Makefile
include drivers/Makefile
include libc/Makefile
include include/Makefile

%.o: %.s
	-@echo "	AS	$@" && $(AS) $(ASFLAGS) -o $@ $^

%.o: %.c
	-@echo "	CC	$@" && $(CC) $(CFLAGS) -c -o $@ $^

all: nImage

.kinfo.c:
	-@echo "	MK	$@" && \
		rm -f $@ && \
		echo "const char * UNAME_M = \"$(ARCH)\";" >> $@ && \
		echo "const char * UNAME_R = \"$(KSTRING)\";" >> $@

nImage: nImage-$(ARCH)

nImage-$(ARCH): $(TRACK) $(KOBJ)
	@echo "	LD	$@" && $(CC) $(LDFLAGS) -o $@ $(KOBJ)

test-$(ARCH): nImage-$(ARCH)
	@qemu-system-$(ARCH) -kernel $^ -M isapc,accel=tcg -cpu $(QEMU_CPU) -m 8M $(QEMU_OPTS)

clean:
	-@echo "	CL	.*.c" && find . -name '.*.c' -delete
	-@echo "	CL	*.o" && find . -name '*.o' -delete
	-@echo "	CL	nImage-*" && find . -name 'nImage-*' -delete
