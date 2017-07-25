#include <phidias.h>
#include <mmio.h>
#include <drivers/clock-mpcore.h>

void clock_mpcore_setup(const memarea *area) {
	if (cpu_number == 0) {
		mmio_write32(area->vaddr + MPCORE_GLOBALTIMER_CONTROL, 1);
	}
}

uint64_t clock_mpcore_read(const memarea *area) {
	uint64_t clock_value, clock_value_high;

	do {
		clock_value_high = mmio_read32(area->vaddr + MPCORE_GLOBALTIMER_VALUE_HIGH);
		clock_value = mmio_read32(area->vaddr + MPCORE_GLOBALTIMER_VALUE_LOW);
	} while (clock_value_high != mmio_read32(area->vaddr + MPCORE_GLOBALTIMER_VALUE_HIGH));

	return clock_value | (clock_value_high << 32);
}
