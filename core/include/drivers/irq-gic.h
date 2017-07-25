#ifndef	__ASSEMBLER__

# include <asm/gic.h>

struct memarea_;

extern void irq_gic_setup(const struct memarea_ *area);
extern uint32_t irq_gic_get_irq_raw(const struct memarea_ *area);
extern void irq_gic_ack_irq(const struct memarea_ *area, uint32_t);
extern void irq_gic_disable_irq(const struct memarea_ *area, uint32_t);
extern void irq_gic_raise_ipi(const struct memarea_ *area, uint32_t);

#endif
