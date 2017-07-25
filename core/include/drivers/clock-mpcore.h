#ifndef	_PHIDIAS_DRIVERS_CLOCK_MPCORE_H_
#define	_PHIDIAS_DRIVERS_CLOCK_MPCORE_H_

# include <asm/mpcore.h>

struct memarea_;

extern void clock_mpcore_setup(const struct memarea_ *area);
extern uint64_t clock_mpcore_read(const struct memarea_ *area);

#endif
