#ifndef	_PHIDIAS_DRIVERS_CLOCK_H_
#define	_PHIDIAS_DRIVERS_CLOCK_H_

# if defined(CLOCK_DRIVER_TSC)

# include <drivers/clock-tsc.h>

static inline void clock_setup(void) {
	clock_tsc_setup();
}

static inline uint64_t clock_read(void) {
	return clock_tsc_read();
}

# elif defined(CLOCK_DRIVER_MPCORE)

# include <drivers/clock-mpcore.h>

static inline void clock_setup(void) {
	clock_mpcore_setup(core_memarea(MEMAREA_MPCORE));
}

static inline uint64_t clock_read(void) {
	return clock_mpcore_read(core_memarea(MEMAREA_MPCORE));
}

# elif defined(CLOCK_DRIVER_ARM_GENERIC)

# include <drivers/clock-arm-generic.h>

static inline void clock_setup(void) {
	clock_arm_generic_setup();
}

static inline uint64_t clock_read(void) {
	return clock_arm_generic_read();
}

# elif defined(CLOCK_DRIVER_PIC32)

# include <drivers/clock-pic32.h>

static inline void clock_setup(void) {
	clock_pic32_setup();
}

static inline uint64_t clock_read(void) {
	return clock_pic32_read();
}

# else

# warning No clock driver selected!

static inline void clock_setup(void) {}
static inline uint64_t clock_read(void) { return 0L; }

# endif

#endif
