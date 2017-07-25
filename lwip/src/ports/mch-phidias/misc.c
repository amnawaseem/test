#include "mch.h"

size_t printf(const char *format, ...) {
	__builtin_va_list va;
	u64_t arg;
	int i;

	__builtin_va_start(va, format);

	while (*format) {
		if (*format != '%') {
			drv_serial_outch(*format);
			++format;
			continue;
		}
		++format;
		arg = __builtin_va_arg(va, u64_t);
		for (i = 60; i >= 0; i -= 4) {
			u8_t nibble = (arg >> i) & 0xf;
			drv_serial_outch(nibble > 9 ? 'a' + (nibble - 10) : '0' + nibble);
		}
	}

	__builtin_va_end(va);

	return 0;
}

sys_prot_t sys_arch_protect() {
	return 0;
}

void sys_arch_unprotect(sys_prot_t val) {
}

void memset(void *buf, u8_t val, u32_t size) {
	u8_t *bufp;

	for (bufp = buf; size; size--, bufp++)
		*bufp = val;
}

void memcpy(void *dest, const void *src, u32_t size) {
	u8_t *destp;
	const u8_t *srcp;

	for (destp = dest, srcp = src; size; destp++, srcp++, size--)
		*destp = *srcp;
}

s32_t memcmp(const u8_t *buf1, const u8_t *buf2, size_t size) {
	size_t len = 0;

	for (; (len < size) && (*buf1 == *buf2);
		len++, buf1++, buf2++)
		;

	if (len == size) return 0;

	return (s32_t)*buf1 - (s32_t)*buf2;
}

size_t strlen(const char *str) {
	size_t len = 0;

	while (*str)
		++len, ++str;

	return len;
}

u16_t lwip_htons(u16_t v) {
	return ((v >> 8) | (v << 8)) & 0xffff;
}

u32_t lwip_htonl(u32_t v) {
	return ((v&0xff)<<24) | ((v&0xff00)<<8) | ((v&0xff0000)>>8) | ((v&0xff000000)>>24);
}

u16_t lwip_ntohs(u16_t v) { return lwip_htons(v); }
u32_t lwip_ntohl(u32_t v) { return lwip_htonl(v); }

u32_t sys_now() {
	return 0;
}

u32_t sys_rand() {
	u32_t tval;

	asm volatile("mrs %0, CNTVCT_EL0" : "=r" (tval));

	tval = (tval-1) * tval;

	return tval;
}
