#ifndef	_PHIDIAS_DRIVERS_TIMER_PIC32_H_
#define	_PHIDIAS_DRIVERS_TIMER_PIC32_H_

# include <asm/timer-pic32.h>
# include <debug.h>

struct memarea_;
struct timer_;

extern void timer_pic32_setup(const struct memarea_ *area);
extern void timer_pic32_program(const struct memarea_ *area, struct timer_ *);
extern void timer_pic32_interrupt(void);
extern void _timer_interrupt (void);

static inline uint32_t timer_pic32_get_prescaler(uint32_t prescaler){
	switch(prescaler){
		case 1:
			return 0;
		case 2:
			return 1;
		case 4:
			return 2;
		case 8:
			return 3;
		case 16:
			return 4;
		case 32:
			return 5;
		case 64:
			return 6;
		case 256:
			return 7;
		default:
			printf("unsenseful prescaler value: %d", prescaler);
			return 0xffff;
	}
}
#endif
