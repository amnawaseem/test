#ifndef	_PHIDIAS_DRIVERS_TIMER_LAPIC_H_
#define	_PHIDIAS_DRIVERS_TIMER_LAPIC_H_

# include <asm/lapic.h>

struct memarea_;
struct timer_;

extern void timer_lapic_setup(const struct memarea_ *area);
extern void timer_lapic_program(const struct memarea_ *area, struct timer_ *);
extern uint32_t timer_lapic_interrupt(const struct memarea_ *area);

#endif
