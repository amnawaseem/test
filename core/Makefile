PWD := $(shell pwd)

ifeq ($(V),1)
VE := @\#
VC :=
else
VE := @
VC := @
endif

# --------------------------------------------------------------------------

ifneq ($(O),)
override O := $(shell realpath $(patsubst %/,%,$(O)) 2>/dev/null)

ifeq ($(O),)
$(error Nonexistent or invalid build directory)
endif

include $(O)/Makeconf
include arch/$(ARCH)/Makeconf.arch
include plat/$(PLATFORM)/Makeconf.plat

TARGET_GCC := $(TARGET_PREFIX)gcc
TARGET_OBJCOPY := $(TARGET_PREFIX)objcopy

COMMON_CFLAGS := -Wall -Wextra -O0 -g2 -fno-pic -fno-PIC -no-pie
COMMON_CFLAGS += -fno-zero-initialized-in-bss
COMMON_CFLAGS += -nostdlib -nostdinc
COMMON_CFLAGS += -ffreestanding -fno-builtin -fno-common
COMMON_CFLAGS += -fno-builtin-printf
COMMON_CFLAGS += -Wl,--build-id=none
COMMON_CFLAGS += -I $(O)/include
COMMON_CFLAGS += -I $(PWD)/include
COMMON_CFLAGS += -I $(PWD)/../abi

TARGET_CFLAGS := $(COMMON_CFLAGS) $(ARCH_CFLAGS) $(PLAT_CFLAGS)

EXEC_BASE_ADDR := $(shell ../xml/tools/xmlq -s0 -o$(O) /scenario/hypervisor:exec_base 2>/dev/null)
ifeq ($(EXEC_BASE_ADDR),)
$(error Unspecified XML attribute /scenario/hypervisor:exec_base)
endif
LDSCRIPT_FLAGS := -Wl,--defsym,PHIDIAS_VADDR_BASE=$(EXEC_BASE_ADDR),-T$(O)/phidias.ld

OFFSET_TOOLS := tools/offsets.awk tools/offsets2.awk tools/offsets.c

HDR := $(shell find include -name "*.h")
HDR += $(shell find ../abi -name "*.h")
HDR += $(O)/include/config.h $(O)/include/asm/offsets.h

CSRC := main.c misc_ops.c schedule.c interrupts.c timequeue.c
CSRC += fault.c memarea.c hypercall.c xcore.c
CSRC += $(if $(FEATURE_TRACE),trace.c)
CSRC += $(if $(FEATURE_DEBUG),debug.c)
CSRC += $(if $(FEATURE_MULTIPLEXER),multiplexer.c)

CSRC += emulate/core.c
CSRC += $(if $(EMULATE_MEMORY32),emulate/memory.c)
CSRC += $(if $(EMULATE_VTLB),emulate/vtlb.c)
CSRC += $(if $(EMULATE_UART_PL011),emulate/uart-pl011.c)
CSRC += $(if $(EMULATE_UART_16550),emulate/uart-16550.c)
CSRC += $(if $(EMULATE_UART_PIC32),emulate/uart-pic32.c)
CSRC += $(if $(EMULATE_IRQ_GIC),emulate/irq/gic.c)
CSRC += $(if $(EMULATE_IRQ_LAPIC),emulate/irq/lapic.c)
CSRC += $(if $(EMULATE_TIMER_SP804),emulate/timer/sp804.c)
CSRC += $(if $(EMULATE_TIMER_MPCORE),emulate/timer/mpcore.c)
CSRC += $(if $(EMULATE_TIMER_ARMCP14),emulate/timer/armcp14.c)

CSRC += drivers/uart.c
CSRC += $(if $(UART_DRIVER_PL011),drivers/uart-pl011.c)
CSRC += $(if $(UART_DRIVER_16550),drivers/uart-16550.c)
CSRC += $(if $(UART_DRIVER_PIC32),drivers/uart-pic32.c)
CSRC += drivers/timer.c
CSRC += $(if $(TIMER_DRIVER_LAPIC),drivers/timer-lapic.c)
CSRC += $(if $(TIMER_DRIVER_MPCORE),drivers/timer-mpcore.c)
CSRC += $(if $(TIMER_DRIVER_ARM_GENERIC),drivers/timer-arm-generic.c)
CSRC += $(if $(TIMER_DRIVER_PIC32),drivers/timer-pic32.c)
CSRC += drivers/irq.c
CSRC += $(if $(IRQ_DRIVER_LAPIC),drivers/irq-lapic.c)
CSRC += $(if $(IRQ_DRIVER_MPCORE),drivers/irq-mpcore.c)
CSRC += $(if $(IRQ_DRIVER_BCM283X),drivers/irq-bcm283x.c)
CSRC += $(if $(IRQ_DRIVER_GIC),drivers/irq-gic.c)
CSRC += $(if $(IRQ_DRIVER_PIC32),drivers/irq-pic32.c)
# drivers/clock
CSRC += $(if $(CLOCK_DRIVER_TSC),drivers/clock-tsc.c)
CSRC += $(if $(CLOCK_DRIVER_MPCORE),drivers/clock-mpcore.c)
CSRC += $(if $(CLOCK_DRIVER_ARM_GENERIC),drivers/clock-arm-generic.c)
CSRC += $(if $(CLOCK_DRIVER_PIC32),drivers/clock-pic32.c)
SSRC :=

CSRC += $(patsubst %.c,arch/$(ARCH)/%.c,$(ARCH_CSRC))
SSRC += $(patsubst %.S,arch/$(ARCH)/%.S,$(ARCH_SSRC))

CSRC += $(patsubst %.c,plat/$(PLATFORM)/%.c,$(PLAT_CSRC))
SSRC += $(patsubst %.S,plat/$(PLATFORM)/%.S,$(PLAT_SSRC))

COBJ := $(patsubst %.c,$(O)/%.o,$(CSRC))
SOBJ := $(patsubst %.S,$(O)/%.o,$(SSRC))

DIRS := $(sort $(dir $(COBJ) $(SOBJ))) $(O)/include $(O)/include/arch $(O)/include/plat $(O)/include/asm

$(O)/phidias.elf: $(COBJ) $(SOBJ) $(O)/invoke_gcc.cmd $(O)/phidias.ld
	$(VE)echo -e "GEN\tstamp.S"
	$(VC)tools/stamp $(TARGET_PREFIX) > $(O)/stamp.S
	$(VE)echo -e "CC\tstamp.{S -> o}"
	$(VC)$(TARGET_GCC) $(TARGET_CFLAGS) -c $(O)/stamp.S -o $(O)/stamp.o
	$(VE)echo -e "LINK\t$$(basename $@)"
	$(VC)$(TARGET_GCC) $(TARGET_CFLAGS) $(filter %.o,$^) $(O)/stamp.o $(LDSCRIPT_FLAGS) -o $@
	$(VE)echo -e "CHECK\t$$(basename $@)"
	$(VC)BSS_SYMBOLS="$$(nm $@ | grep __phidias_bss_ | cut -d' ' -f1 | sort -u)"; \
	if [ $$(grep -c . <<< "$$BSS_SYMBOLS") -gt 1 ]; then \
		echo "ERROR: .bss section detected"; \
		exit 1; \
	fi

LDSCRIPT := $(wildcard arch/$(ARCH)/phidias.ld phidias.ld)

$(O)/phidias.ld: $(firstword $(LDSCRIPT))
	cp $^ $@

$(O)/include/asm/offsets.h: %: $(filter-out $(O)/include/asm/offsets.h, $(HDR)) $(OFFSET_TOOLS) | $(O)/include/arch $(O)/include/plat $(O)/include/asm
	$(VE)echo -e "GEN\toffsets.h"
	$(VC)cat tools/offsets.c >$(basename $@).0.h
	$(VC)$(TARGET_GCC) $(TARGET_CFLAGS) -x c -E tools/offsets.c >>$(basename $@).0.h
	$(VC)gawk -f tools/offsets.awk $(basename $@).0.h > $(basename $@).0.c
	$(VC)$(TARGET_GCC) $(TARGET_CFLAGS) -c $(basename $@).0.c -o $(basename $@).0.o
	$(VC)$(TARGET_OBJCOPY) -Obinary -j.rodata_off $(basename $@).0.o $(basename $@).off.raw
	$(VC)hexdump -v -e '1/4 "0x%x\n"' $(basename $@).off.raw > $(basename $@).off.x
	$(VC)$(TARGET_OBJCOPY) -Obinary -j.rodata_size $(basename $@).0.o $(basename $@).size.raw
	$(VC)hexdump -v -e '1/4 "0x%x\n"' $(basename $@).size.raw > $(basename $@).size.x
	$(VC)gawk -f tools/offsets2.awk $(basename $@).off.x $(basename $@).size.x $(basename $@).0.h > $@
	$(VC)rm -f $(basename $@).0.* $(basename $@).off.* $(basename $@).size.*

$(COBJ): $(O)/%.o: %.c $(HDR) | $(DIRS)
	$(VE)V=$(patsubst %.c,"%.{c -> o}",$<); echo -e "CC\t$$V"
	$(VC)$(TARGET_GCC) $(TARGET_CFLAGS) -c $< -o $@

$(SOBJ): $(O)/%.o: %.S $(O)/include/asm/offsets.h $(HDR) | $(DIRS)
	$(VE)V=$(patsubst %.S,"%.{S -> o}",$<); echo -e "CC\t$$V"
	$(VC)$(TARGET_GCC) $(TARGET_CFLAGS) -c $< -o $@

$(filter-out $(O)/include/arch $(O)/include/plat,$(DIRS)):
	$(VC)mkdir -p $@

$(O)/include/arch: | $(O)/include
	$(VC)ln -nsfT $$(pwd)/include/arch/$(ARCH) $@

$(O)/include/plat: | $(O)/include
	$(VC)ln -nsfT $$(pwd)/include/plat/$(PLATFORM) $@

$(O)/include/config.h: $(O)/Makeconf arch/$(ARCH)/Makeconf.arch plat/$(PLATFORM)/Makeconf.plat | $(O)/include
	$(VC)cat $^ | awk -f gen_config.awk | make --no-print-directory -f - > $@

$(O)/invoke_gcc.cmd: $(O)/include/config.h
	$(VC)echo "$(TARGET_GCC) $(TARGET_CFLAGS)" > $@

clean:
	rm -f $(SOBJ) $(COBJ) $(O)/stamp.o $(O)/stamp.S $(O)/phidias.elf
	rm -f $(O)/include/config.h $(O)/include/asm/offsets.h
	rm -f $(O)/include/arch $(O)/include/plat $(O)/invoke_gcc.cmd

distclean:
	rm -f $(O)/Makeconf

# --------------------------------------------------------------------------

endif

notarget:
	@echo "Please specify a build directory using O=\$$BUILDDIR."
