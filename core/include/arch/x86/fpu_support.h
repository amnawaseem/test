static inline uint32_t highest_bit(uint32_t value) {
	uint32_t bit;

	asm volatile("bsr %1, %0" : "=r" (bit) : "r" (value));

	return bit;
}

extern uint64_t divide_u64_u64(uint64_t, uint64_t);
extern uint64_t modulo_u64_u64(uint64_t, uint64_t);
