#ifndef _PHIDIAS_ARCH_ARM_CP15_REGS_H_
#define	_PHIDIAS_ARCH_ARM_CP15_REGS_H_

# include <arch/asm/cp15_regs.h>

# define COPROCESSOR_WRITE(code, value)	{				\
        asm volatile("mcr p%c5, %1, %0, cr%c2, cr%c3, %4" ::		\
                "r" (value),						\
                "M" (BITS(code, 12, 4)),				\
                "M" (BITS(code, 8, 4)),					\
                "M" (BITS(code, 4, 4)),					\
                "M" (BITS(code, 0, 4)),					\
		"M" (BITS(code, 16, 4)));				\
}

# define COPROCESSOR_READ(code, var)	{				\
        asm volatile("mrc p%c5, %1, %0, cr%c2, cr%c3, %4" :		\
                "=r" (var) :						\
                "M" (BITS(code, 12, 4)),				\
                "M" (BITS(code, 8, 4)),					\
                "M" (BITS(code, 4, 4)),					\
                "M" (BITS(code, 0, 4)),					\
		"M" (BITS(code, 16, 4)));				\
}

# define COPROCESSOR_WRITE64(code, value) {				\
	asm volatile("mcrr p%c3, %1, %Q0, %R0, cr%c2" :			\
		"r" (value),						\
		"M" (BITS(code, 4, 4)),					\
		"M" (BITS(code, 0, 4)),					\
		"M" (BITS(code, 8, 4)));				\
}

# define COPROCESSOR_READ64(code, value) {				\
	asm volatile("mrrc p%c3, %1, %Q0, %R0, cr%c2" :			\
		"=r" (value) :						\
		"M" (BITS(code, 4, 4)),					\
		"M" (BITS(code, 0, 4)),					\
		"M" (BITS(code, 8, 4)));				\
}

struct vm_cpu_;

extern void coproc_write(const struct vm_cpu_ *, uint32_t coproc, uint32_t code, uint32_t reg);
extern uint32_t coproc_read(const struct vm_cpu_ *, uint32_t coproc, uint32_t code);

#endif
