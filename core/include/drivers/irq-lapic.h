#ifndef	_PHIDIAS_DRIVERS_IRQ_LAPIC_H_
#define	_PHIDIAS_DRIVERS_IRQ_LAPIC_H_

# include <asm/lapic.h>

extern void irq_lapic_setup(const memarea *area_lapic, const memarea *area_ioapic);
extern uint32_t irq_lapic_get_irq_raw(const memarea *area_lapic, const memarea *area_ioapic);
extern void irq_lapic_ack_irq(const memarea *area_lapic, const memarea *area_ioapic, uint32_t);
extern void irq_lapic_disable_irq(const memarea *area_lapic, const memarea *area_ioapic, uint32_t);
extern void irq_lapic_raise_ipi(const memarea *area_lapic, uint32_t);

#endif
