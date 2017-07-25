#ifndef	_PHIDIAS_DRIVERS_TIMER_H_
#define	_PHIDIAS_DRIVERS_TIMER_H_

struct timer_;

# if defined(TIMER_DRIVER_LAPIC)

# include <drivers/timer-lapic.h>

static inline void timer_setup(void) {
	timer_lapic_setup(core_memarea(MEMAREA_LAPIC));
}

static inline void timer_program(struct timer_ *timer) {
	timer_lapic_program(core_memarea(MEMAREA_LAPIC), timer);
}

# elif defined(TIMER_DRIVER_MPCORE)

# include <drivers/timer-mpcore.h>

static inline void timer_setup(void) {
	timer_mpcore_setup(core_memarea(MEMAREA_MPCORE));
}

static inline void timer_program(struct timer_ *timer) {
	timer_mpcore_program(core_memarea(MEMAREA_MPCORE), timer);
}

# elif defined(TIMER_DRIVER_ARM_GENERIC)

# include <drivers/timer-arm-generic.h>

static inline void timer_setup(void) {
	timer_arm_generic_setup();
}

static inline void timer_program(struct timer_ *timer) {
	timer_arm_generic_program(timer);
}

# elif defined(TIMER_DRIVER_PIC32)

# include <drivers/timer-pic32.h>

static inline void timer_setup(void) {
	timer_pic32_setup(core_memarea(MEMAREA_TIMER_PIC32));
}

static inline void timer_program(struct timer_ *timer) {
	timer_pic32_program(core_memarea(MEMAREA_TIMER_PIC32),timer);
}

# else
# warning No timer driver selected!

static inline void timer_setup(void) {}
static inline void timer_program(struct timer_ *timer) { (void)timer; }

# endif

extern uint32_t timer_interrupt(void);

#endif
