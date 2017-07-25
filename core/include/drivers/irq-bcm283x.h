#ifndef	_PHIDIAS_DRIVERS_IRQ_MPCORE_H_
#define	_PHIDIAS_DRIVERS_IRQ_MPCORE_H_

extern void irq_bcm283x_setup(const memarea *area_irq, const memarea *area_mbox);
extern uint32_t irq_bcm283x_get_irq_raw(const memarea *area_irq, const memarea *area_mbox);
extern void irq_bcm283x_ack_irq(const memarea *area, uint32_t);
extern void irq_bcm283x_disable_irq(const memarea *area, uint32_t);

#endif
