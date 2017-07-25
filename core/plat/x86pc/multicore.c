#include <phidias.h>
#include <mmio.h>
#include <asm/lapic.h>

void platform_boot_cpus() {
#if 0
	const memarea *area_lapic = core_memarea(MEMAREA_LAPIC);
	uint32_t i;

	printf("Bringing up cores...\r\n");

	mmio_write32(area_lapic->vaddr + LAPICREG_ICR_HIGH, 0);
	mmio_write32(area_lapic->vaddr + LAPICREG_ICR_LOW, 0x000c4500);
	for (i = 0x80000; i; i--) asm volatile("nop" ::: "memory");
	mmio_write32(area_lapic->vaddr + LAPICREG_ICR_LOW, 0x000c4666);
	for (i = 0x80000; i; i--) asm volatile("nop" ::: "memory");
	mmio_write32(area_lapic->vaddr + LAPICREG_ICR_LOW, 0x000c4666);
#endif
}
