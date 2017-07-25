#include <phidias.h>
#include <mmio.h>
#include <drivers/clock-tsc.h>

void clock_tsc_setup() {
}

uint64_t clock_tsc_read() {
	uint64_t clock_value;

#if	defined(__i386)
	asm volatile("rdtsc" : "=A" (clock_value));
#elif	defined(__x86_64)
	asm volatile("rdtsc\n\tshl $32, %%rdx\n\tor %%rdx, %%rax" : "=a" (clock_value) :: "rdx");
#else
# error	No TSC on this architecture.
#endif

	return clock_value;
}
