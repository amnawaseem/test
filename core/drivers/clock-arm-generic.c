#include <phidias.h>
#include <mmio.h>
#include <specification.h>
#include <drivers/clock-arm-generic.h>

void clock_arm_generic_setup() {
	if (cpu_number == 0) {
		clock_arm_generic_el2_control_access(0);
	}
}

uint64_t clock_arm_generic_read() {
	return clock_arm_generic_phys_read();
}
