#include <phidias.h>
#include <bootup.h>
#include <mmio.h>
#include <memarea.h>

uint32_t __shared multicore_boot_aux = 1;

void platform_boot_cpus() {
	// multicore_boot_aux = 2;
	asm volatile("dc cvac, %0\n\tdsb sy" :: "r" (&multicore_boot_aux) : "memory");
	asm volatile("sev" ::: "memory");
}
