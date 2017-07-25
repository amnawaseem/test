#ifndef	__ASSEMBLER__

static inline uint32_t phidias_hypercall1(uint32_t r0) {
	uint32_t r;
	asm volatile("mov r0, %1\n\t"
			"svc #0\n\t"
			"mov %0, r0"
		: "=r" (r) : "r" (r0) : "r0");
	return r;
}

static inline uint32_t phidias_hypercall2(uint32_t r0, uint32_t r1) {
	uint32_t r;
	asm volatile("mov r0, %1\n\t"
			"mov r1, %2\n\t"
			"svc #0\n\t"
			"mov %0, r0"
		: "=r" (r) : "r" (r0), "r" (r1) : "r0", "r1");
	return r;
}

#endif
