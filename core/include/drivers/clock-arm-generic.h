#ifndef	_PHIDIAS_DRIVERS_CLOCK_ARM_GENERIC_H_
#define	_PHIDIAS_DRIVERS_CLOCK_ARM_GENERIC_H_

extern void clock_arm_generic_setup(void);
extern uint64_t clock_arm_generic_read(void);

# if defined(ARCH_ARM64)
static inline void clock_arm_generic_el2_control_access(uint32_t value) {
	asm volatile("msr CNTHCTL_EL2, %0" :: "r" (value));
}

static inline uint64_t clock_arm_generic_phys_read(void) {
	uint64_t clock_value;
	asm volatile("mrs %0, CNTPCT_EL0" : "=r" (clock_value));
	return clock_value;
}
# elif defined(ARCH_ARM)
#  include <arch/cp15_regs.h>

static inline void clock_arm_generic_el2_control_access(uint32_t value) {
	COPROCESSOR_WRITE(COPROC_CODE_CNTHCTL, value);
}

static inline uint64_t clock_arm_generic_phys_read(void) {
	uint64_t value;
	COPROCESSOR_READ64(COPROC_CODE64_CNTPCT, value);
	return value;
}
# else
#  error No ARM generic clock accessor functions for this architecture!
# endif

#endif
