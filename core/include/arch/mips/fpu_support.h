#ifndef	__ASSEMBLER__

extern uint64_t divide_u64_u64(uint64_t, uint64_t);
extern uint64_t modulo_u64_u64(uint64_t, uint64_t);

static inline uint32_t highest_bit(uint32_t value) {
	uint32_t bit;

	asm volatile("clz %0, %1\n\tsub %0, $0, %0\n\taddi %0, %0, 31" : "=r" (bit) : "r" (value));

	return bit;
}

#endif
