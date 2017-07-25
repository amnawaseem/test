#ifndef	_PHIDIAS_DRIVERS_TIMER_MPCORE_H_
#define	_PHIDIAS_DRIVERS_TIMER_MPCORE_H_

# include <asm/mpcore.h>

struct memarea_;
struct timer_;

extern void timer_mpcore_setup(const struct memarea_ *area);
extern void timer_mpcore_program(const struct memarea_ *area, struct timer_ *);
extern uint32_t timer_mpcore_interrupt(const struct memarea_ *area);

#endif
