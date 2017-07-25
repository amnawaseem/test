#ifndef	_PHIDIAS_DRIVERS_CLOCK_PIC32_H_
#define	_PHIDIAS_DRIVERS_CLOCK_PIC32_H_

# include <asm/timer-pic32.h>

struct memarea_;

extern void clock_pic32_setup(void);
extern uint64_t clock_pic32_read(void);
extern void clock_pic32_interrupt(void);
extern void _clock_interrupt (void);

#endif
