#ifndef	_PHIDIAS_EMULATE_IRQ_H_
#define	_PHIDIAS_EMULATE_IRQ_H_

# include <vm.h>
# include <timequeue.h>

#define	NO_PENDING_INTERRUPT	~0U

# if defined(EMULATE_IRQ_LAPIC)
struct emulate_irq_lapic_ {
	uint32_t	destination;
	uint32_t	destination_mode;
	uint32_t	lvt[6];
	uint32_t	timer_initial;
	uint64_t	timer_reference_value;
	uint32_t	timer_current;
	uint32_t	timer_divider;
	uint32_t	eff_timer_divisor;
	uint32_t	frequency;
	uint32_t	isr[256 >> 5];
	uint32_t	irr[256 >> 5];
	struct queue_timer_	event;

	uint32_t	ioregsel;
	uint32_t	ioredtbl[48];
};
typedef struct emulate_irq_lapic_	emulate_irq_lapic;

extern void emulate_irq_lapic_initialize(const struct vm_cpu_ *vm_cpu_em, emulate_irq_lapic *lapic);
extern void emulate_irq_lapic_store(const struct vm_cpu_ *vm_cpu_em, emulate_irq_lapic *lapic,
                        uint32_t bar, uintptr_t offset, uint32_t value);
extern uint32_t emulate_irq_lapic_load(const struct vm_cpu_ *vm_cpu_em, emulate_irq_lapic *lapic,
                        uint32_t bar, uintptr_t offset);
extern uint32_t emulate_irq_lapic_activate_highest_interrupt(const struct vm_cpu_ *vm_cpu_em, emulate_irq_lapic *lapic);
extern void emulate_irq_lapic_set_interrupt_pending(const struct vm_cpu_ *vm_cpu_em, emulate_irq_lapic *lapic, uint32_t interrupt);
# endif /*EMULATE_IRQ_LAPIC */

# if defined(EMULATE_IRQ_GIC)
struct emulate_irq_gic_distributor_ {
	uint32_t	lock;
	uint32_t	control;
	uint32_t	pending_bits[(1024 - 32) >> 5];
	uint32_t	masked_bits[(1024 - 32) >> 5];
	uint32_t	priorities[(1024 - 32) >> 5];
	uint32_t	target_bits[(1024 - 32) >> 5];
};
typedef struct emulate_irq_gic_distributor_	emulate_irq_gic_distributor;

struct emulate_irq_gic_ {
	uint32_t	control;		// need ITLinesNum
	uint32_t	pmr;
	uint32_t	bpr;
	uint32_t	active_irq;
	uint32_t	private_pending_bits[32 >> 5];
	uint32_t	private_masked_bits[32 >> 5];
	uint32_t	private_priorities[32 >> 5];
	uint8_t		sgi_source[16];
	emulate_irq_gic_distributor	*distributor;
	uint32_t	virt_listregs[4];
	uint32_t	virt_control;
};
typedef struct emulate_irq_gic_	emulate_irq_gic;

extern void emulate_irq_gic_initialize(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic);
extern void emulate_irq_gic_store(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic,
                        uint32_t bar, uintptr_t offset, uint32_t value);
extern uint32_t emulate_irq_gic_load(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic,
                        uint32_t bar, uintptr_t offset);
extern uint32_t emulate_irq_gic_activate_highest_interrupt(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic);
extern void emulate_irq_gic_set_interrupt_pending(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic, uint32_t interrupt);

extern uint32_t emulate_irq_gicve_activate_highest_interrupt(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic);
extern void emulate_irq_gicve_save_state(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic);
extern void emulate_irq_gicve_load_state(const struct vm_cpu_ *vm_cpu_em, emulate_irq_gic *gic);
# endif /* EMULATE_IRQ_GIC */

static inline uint32_t emulate_irq_activate_highest_interrupt(const struct vm_cpu_ *vm_cpu_em) {
	if (vm_cpu_em->irq_emulate == NULL) {
		return NO_PENDING_INTERRUPT;
	}

	switch (vm_cpu_em->irq_emulate->type) {
# if defined(EMULATE_IRQ_GIC)
	case EMULATE_TYPE_IRQ_GIC:
		return emulate_irq_gic_activate_highest_interrupt(vm_cpu_em, vm_cpu_em->irq_emulate->control.irq_gic);
	case EMULATE_TYPE_IRQ_GIC_VIRTEXT:
		return emulate_irq_gicve_activate_highest_interrupt(vm_cpu_em, vm_cpu_em->irq_emulate->control.irq_gic);
# endif
# if defined(EMULATE_IRQ_LAPIC)
	case EMULATE_TYPE_IRQ_LAPIC:
		return emulate_irq_lapic_activate_highest_interrupt(vm_cpu_em, vm_cpu_em->irq_emulate->control.irq_lapic);
# endif
	}

	return NO_PENDING_INTERRUPT;
}

static inline void emulate_irq_set_interrupt_pending(const struct vm_cpu_ *vm_cpu_em, uint32_t interrupt) {
	if (vm_cpu_em->irq_emulate == NULL) {
		return;
	}

	switch (vm_cpu_em->irq_emulate->type) {
# if defined(EMULATE_IRQ_GIC)
	case EMULATE_TYPE_IRQ_GIC:
	case EMULATE_TYPE_IRQ_GIC_VIRTEXT:
		emulate_irq_gic_set_interrupt_pending(vm_cpu_em, vm_cpu_em->irq_emulate->control.irq_gic, interrupt);
		return;
# endif
# if defined(EMULATE_IRQ_LAPIC)
	case EMULATE_TYPE_IRQ_LAPIC:
		emulate_irq_lapic_set_interrupt_pending(vm_cpu_em, vm_cpu_em->irq_emulate->control.irq_lapic, interrupt);
# endif
	}
}

#endif /* _PHIDIAS_EMULATE_IRQ_H_ */
