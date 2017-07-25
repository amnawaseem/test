#include <phidias.h>
#include <bootup.h>
#include <mmio.h>
#include <plat/platform_regs.h>
#include <arch/sec_support.h>

void platform_boot_cpus() {
	const memarea *sysregs_area = core_memarea(MEMAREA_CS7A);
	const memarea *mpcore_area = core_memarea(MEMAREA_MPCORE);

	mmio_write32(mpcore_area->vaddr + 0x008, 0);
	mmio_write32(sysregs_area->vaddr + SYSREGS_BOOTUP_VECTOR,
			core_physical_address((uintptr_t)&__boot));
	asm volatile("dsb sy" ::: "memory");

	smc_call(0);
}
