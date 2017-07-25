#ifndef	__ASSEMBLER__

# include <asm/irq-pic32.h>

extern void irq_pic32_setup(const memarea *area);
extern uint32_t irq_pic32_get_irq_raw(const memarea *area);
extern void irq_pic32_ack_irq(const memarea *area, uint32_t);
extern void irq_pic32_disable_irq(const memarea *area, uint32_t);
extern void irq_pic32_enable_irq(const memarea *area, uint32_t number, uint8_t prio, char *isr_ptr);
extern void irq_pic32_enable_irq_vm(const memarea *area, uint32_t number, uint8_t prio,
					uint32_t offset, uint32_t guest_id);

#endif
