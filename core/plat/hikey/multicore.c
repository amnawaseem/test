#include <phidias.h>
#include <bootup.h>
#include <mmio.h>
#include <memarea.h>

extern void psci_call(uint64_t, uint64_t, uint64_t, uint64_t);

void platform_boot_cpus() {
	uint32_t i;

	for (i = 1; i < 8; i++) {
		printf("PSCI CPU_ON for core #%d\r\n", i);

		psci_call(0xc4000003,
				((i & 4) ? 0x100 : 0) | (i & 3),
				core_physical_address((uintptr_t)&__boot),
				0);
	}

	{
		volatile uint32_t *tmmio = (volatile uint32_t *)core_memarea(MEMAREA_THERMAL)->vaddr;
		volatile int i;

		printf("Hi6220 Clks: %x\r\n", tmmio[0x230 >> 2]);
		tmmio[0x230 >> 2] = 0x1000;
		for (i = 16777215; i; i--) ;

		printf("Hi6220 Temp: %x %x %x\r\n",
			tmmio[0x70c >> 2], tmmio[0x710 >> 2], tmmio[0x728 >> 2]);

		tmmio[0x710 >> 2] = 1;
		for (i = 16777215; i; i--) ;

		printf("Hi6220 Temp: %x %x %x\r\n",
			tmmio[0x70c >> 2], tmmio[0x710 >> 2], tmmio[0x728 >> 2]);

		tmmio[0x704 >> 2] = 0xbfbfbfbf;	// IRQ thresholds = 90°C each
		tmmio[0x708 >> 2] = 0xbf;	// RESET threshold = 90°C

		tmmio[0x71c >> 2] = 1;		// enable RESET
		tmmio[0x718 >> 2] = 1;		// enable IRQ
		tmmio[0x714 >> 2] = 1;		// enable IRQ
	}

	/* desperately trying to get MMC2 up with recent Lx+DTB */
	if (0) {
		volatile uint32_t *xva = (volatile uint32_t *)core_memarea(MEMAREA_CATCHALL)->vaddr;

		printf("DWMMC2 V: %x\r\n", xva[0x0723f06c >> 2]);
		printf("GPIO0 DIR: %x\r\n", xva[0x08011400 >> 2]);
		printf("SYSCTL CK0: %x %x\r\n", xva[0x07030004 >> 2], xva[0x07030400 >> 2]);
		printf("SYSCTL CK1: %x %x\r\n", xva[0x07030200 >> 2], xva[0x07030270 >> 2]);
		printf("SYSCTL CK2: %x %x\r\n", xva[0x0703049c >> 2]);
		printf("RESETS: %x\r\n", xva[0x07030304 >> 2]);

		//xva[0x08011400 >> 2] &= ~0x200;
		//for (i = 16777215; i; i--) ;

		xva[0x07030304 >> 2] = 0x7;
		xva[0x07030004 >> 2] |= 0x10;
		xva[0x07030400 >> 2] |= 0x9000;
		xva[0x07030200 >> 2] |= 0x4;
		xva[0x07030270 >> 2] |= 0x808;
		for (i = 16777215; i; i--) ;

		printf("DWMMC2 V: %x\r\n", xva[0x0723f06c >> 2]);
		printf("GPIO0 DIR: %x\r\n", xva[0x08011400 >> 2]);
		printf("SYSCTL CK0: %x %x\r\n", xva[0x07030004 >> 2], xva[0x07030400 >> 2]);
		printf("SYSCTL CK1: %x %x\r\n", xva[0x07030200 >> 2], xva[0x07030270 >> 2]);
		printf("SYSCTL CK2: %x %x\r\n", xva[0x0703049c >> 2]);
	}
}
