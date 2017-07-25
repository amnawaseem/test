#ifndef	_PHIDIAS_DRIVERS_TIMER_ARM_GENERIC_H_
#define	_PHIDIAS_DRIVERS_TIMER_ARM_GENERIC_H_

//include <asm/timer-arm-generic.h>

struct memarea_;
struct timer_;

extern void timer_arm_generic_setup(void);
extern void timer_arm_generic_program(struct timer_ *);
extern uint32_t timer_arm_generic_interrupt(void);

# if defined(ARCH_ARM64)
static inline void timer_arm_generic_el2_set_control(uint32_t value) {
	asm volatile("msr CNTHP_CTL_EL2, %0" :: "r" (value));
}

static inline void timer_arm_generic_el2_set_comparator(uint64_t value) {
	asm volatile("msr CNTHP_CVAL_EL2, %0" :: "r" (value));
}
# elif defined(ARCH_ARM)
#  include <arch/cp15_regs.h>

static inline void timer_arm_generic_el2_set_control(uint32_t value) {
	COPROCESSOR_WRITE(COPROC_CODE_CNTHP_CTL, value);
}

static inline void timer_arm_generic_el2_set_comparator(uint64_t value) {
	COPROCESSOR_WRITE64(COPROC_CODE_CNTHP_CVAL, value);
}
# else
#  error No EL2 counter accessor functions for this architecture!
# endif

#endif
