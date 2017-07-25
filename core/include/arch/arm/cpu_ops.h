#ifndef	_PHIDIAS_ARCH_ARM_CPU_OPS_H_
#define	_PHIDIAS_ARCH_ARM_CPU_OPS_H_

# include <arch/asm/cpu_ops.h>

# include <arch/cp15_regs.h>

extern uint32_t top_of_stack;

static inline void arch_barrier() {
	asm volatile("dsb st" ::: "memory");
}

static inline void cpu_idle() {
	asm volatile("mov sp, %0\n\tcpsie i\n\twfi"
		: : "r" (top_of_stack));
}

static inline void cpu_zombie() {
	asm volatile("cpsid afi\n\twfi\n\twfe\n\tb . - 8");
}

static inline void tlb_flush(uintptr_t address, uint32_t identifier) {
	if (address == TLB_ALL_ADDRESSES) {
		COPROCESSOR_WRITE(COPROC_CODE_TLBIASID, identifier);
	} else {
		COPROCESSOR_WRITE(COPROC_CODE_TLBIMVAA, ((address & ~0xfff) | identifier));
	}
}

/**
 * Combine two PSR values to form one. Used during:
 *  - PSR read: read hw_spsr flags, but virtual cpsr mode
 *  - PSR write: write hw_spsr flags, but virtual cpsr mode
 *  - exception return: update hw_spsr with saved flags, but virtual cpsr with saved mode
 */
static inline uint32_t psr_mix_mode_flags(uint32_t mode, uint32_t flags) {
	return (mode & 0x1ff) | (flags & ~0x1ff);
}

extern void cpu_dcache_flush(int do_clean, int do_invalidate);

#endif
