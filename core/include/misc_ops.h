#ifndef	_PHIDIAS_MISC_OPS_H_
#define	_PHIDIAS_MISC_OPS_H_

uint32_t strlen(const char *string);
void memset(void *dest, uint8_t val, uintptr_t length);
void memcpy(void *dest, const void *src, uintptr_t length);

void __attribute__((noreturn)) panic();

#endif
