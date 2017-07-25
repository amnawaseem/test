#ifndef	_PHIDIAS_DRIVERS_IRQ_MPCORE_H_
#define	_PHIDIAS_DRIVERS_IRQ_MPCORE_H_

# include <asm/mpcore.h>

extern void irq_mpcore_setup(const memarea *area);
extern uint32_t irq_mpcore_get_irq_raw(const memarea *area);
extern void irq_mpcore_ack_irq(const memarea *area, uint32_t);
extern void irq_mpcore_disable_irq(const memarea *area, uint32_t);

#endif
