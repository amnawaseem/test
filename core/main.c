#include <phidias.h>
#include <misc_ops.h>
#include <linker_symbols.h>
#include <memarea.h>
#include <version.h>
#include <drivers/uart.h>
#include <drivers/clock.h>
#include <drivers/timer.h>
#include <drivers/irq.h>
#include <xcore.h>
#include <trace.h>
#include <schedule.h>

uint32_t cpu_number = 0;

void prepare_areas(uint32_t cpu) {
	memcpy(	(void *)_specification.cpus[cpu].memareas[MEMAREA_CORE_RW].vaddr,
		(void *)_specification.cpus[cpu].memareas[MEMAREA_CORE_RWT].vaddr,
		__phidias_data_end - __phidias_data_start);

	memcpy(	(void *)_specification.cpus[cpu].memareas[MEMAREA_CONFIG_RW].vaddr,
		(void *)_specification.cpus[cpu].memareas[MEMAREA_CONFIG_RWT].vaddr,
		_specification.cpus[cpu].memareas[MEMAREA_CONFIG_RWT].size);
}

static void print_banner() {
	if (cpu_number == 0) {
		printf("\e[34;1mPHIDIAS v%d.%3d (git rev %s, toolchain %s/%s)\r\n",
			PHIDIAS_VERSION_MAJOR,
			PHIDIAS_VERSION_MINOR,
			phidias_buildstamp_git_revision,
			gcc_version, binutils_version);
		printf("Build: C %s I %s\e[0m\r\n",
			phidias_buildstamp_time_compile,
			_specification.timestamp);
	}

	printf("\e[34mPHIDIAS CPU #%d up.\e[0m\r\n",
			cpu_number);
}

void __attribute__((section(".text"))) main() {
	if (cpu_number == 0) {
		uart_setup();
	}

	print_banner();

	if (cpu_number == 0) {
		platform_boot_cpus();
	}

	xcore_setup();

	clock_setup();
	timer_setup();
	irq_setup();

	trace_setup();

	sched_start();

	reschedule();
}
