#ifndef _PHIDIAS_ARCH_ARM64_CPU_OPS_H_
#define _PHIDIAS_ARCH_ARM64_CPU_OPS_H_

extern uint64_t top_of_stack;

static inline void arch_barrier() {
	asm volatile("dsb st" ::: "memory");
}

static inline void cpu_idle() {
	asm volatile("mov sp, %0\n\tmsr DAIFClr, #0xf\n\twfi" ::
			"r" (top_of_stack));
}

static inline void cpu_zombie() {
	asm volatile("msr DAIFSet, #0xf\n\twfi\n\twfe\n\tb . - 8" ::: "memory");
}

# define	TLB_ALL_ADDRESSES	~0UL

static inline void tlb_flush(uintptr_t address, uint32_t identifier) {
	if (address == TLB_ALL_ADDRESSES) {
		asm volatile("tlbi vmalle1" ::: "memory");
	} else {
		asm volatile("tlbi vae1, %0" :: "r" ((address >> 12) | ((uint64_t)identifier << 48)));
	}
}

extern void cpu_dcache_flush(int do_clean, int do_invalidate);

#endif
