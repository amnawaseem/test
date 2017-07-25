#include "mch.h"
#include "lwip/mem.h"

static u32_t exec_el;

void go_to_sleep(void) {
	asm volatile("wfi" ::: "memory");
}

void architecture_trigger(u32_t val) {
	if (exec_el == 0) {
		asm volatile("mov x0, #0x9999\n\tmov x1, %0\n\tsvc #0" :: "r" (val) : "x0", "x1");
	} else if (exec_el == 4) {
		asm volatile("mov x0, #0x9999\n\tmov x1, %0\n\thvc #0" :: "r" (val) : "x0", "x1");
	} else {
		printf("No trigger functionality at this EL!\r\n");
	}
}

static void architecture_init_el0(void) {
	printf(" * EL0 set up (nothing to do).\r\n");
}

static void architecture_init_el1(void) {
	u64_t *ptbl;

	ptbl = mem_malloc(8 * 8);
	while ((u64_t)ptbl & 0x18)
		ptbl++;

	ptbl[0] = 0x421;
	ptbl[1] = 0x40000421;
	ptbl[2] = 0x80000421;
	ptbl[3] = 0xc0000421;

	printf(" * (EL1) Activating pagetable at %\r\n", ptbl);

	asm volatile("msr TCR_EL1, %0\n\t"
			"msr TTBR0_EL1, %1\n\t"
			"msr MAIR_EL1, %3\n\t"
			"msr VBAR_EL1, %4\n\ttlbi vmalle1\n\tic iallu\n\tisb\n\t"
			"msr SCTLR_EL1, %2\n\tisb" ::
			"r" (0x00800520), "r" (ptbl), "r" (0x30d01805), "r" (0xff), "r" (0));

	printf(" * EL1 set up.\r\n");
}

void architecture_init(void) {
	asm volatile("mrs %0, CurrentEL" : "=r" (exec_el));

	if (exec_el == 0) {
		architecture_init_el0();
	} else if (exec_el == 4) {
		architecture_init_el1();
	} else {
		printf("ERROR: Unsupported EL %.\r\n", exec_el);
		while (1) { go_to_sleep(); }
	}
}
