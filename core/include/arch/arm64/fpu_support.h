#ifndef	_PHIDIAS_ARCH_ARM64_FPU_SUPPORT_H_
#define	_PHIDIAS_ARCH_ARM64_FPU_SUPPORT_H_

extern uint64_t divide_u64_u64(uint64_t, uint64_t);
extern uint64_t modulo_u64_u64(uint64_t, uint64_t);

static inline uint32_t highest_bit(uint32_t value) {
	uint32_t result;
	asm volatile("clz %w0, %w1" : "=r" (result) : "r" (value));
	return 31 - result;
}

#endif
