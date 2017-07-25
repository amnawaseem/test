#include "mch.h"

#define	SERIAL_BASE		(volatile u32_t *)0xf7113000

static volatile u32_t *serial_mmio = SERIAL_BASE;

void drv_serial_outstr(const char *str) {
	while (*str) {
		drv_serial_outch(*str);
		++str;
	}
}

void drv_serial_outch(char c) {
	while (serial_mmio[6] & 0x20)
		;
	serial_mmio[0] = (u8_t)c;
}
