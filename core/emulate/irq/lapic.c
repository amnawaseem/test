#include <phidias.h>
#include <vm.h>
#include <lock.h>
#include <mmio.h>
#include <memarea.h>
#include <interrupts.h>
#include <xcore.h>
#include <drivers/clock.h>
#include <emulate/core.h>
#include <emulate/irq.h>
#include <asm/lapic.h>
#include <arch/fpu_support.h>

static void lapic_estimate_current(emulate_irq_lapic *lapic);

static void lapic_timer_reconfigure(emulate_irq_lapic *lapic) {
	uint64_t deadline;

	if (!(lapic->lvt[0] & LAPICLVT_MASK)) {
		// interpolate TCCR
		lapic_estimate_current(lapic);

		// calculate scaled TCCR (TDIV)
		deadline = clock_read() + (lapic->timer_current << lapic->eff_timer_divisor);

		// modify event in timerqueue
		if (lapic->event.item.armed) {
			timequeue_delete(&lapic->event);
		}
		lapic->event.item.expiration = deadline;
		timequeue_insert(&lapic->event);

		// printf("EmLapicT %x %x\r\n", clock_read(), deadline);
	}
}

static void lapic_estimate_current(emulate_irq_lapic *lapic) {
	uint64_t now = clock_read();
	uint32_t new_current = lapic->timer_current -
			(now - lapic->timer_reference_value) * 1;

	lapic->timer_reference_value = now;
	lapic->timer_current = new_current;
}

void emulate_irq_lapic_expired(timer *timer_expired) {
	queue_timer *lapic_qtimer = container(queue_timer, item, timer_expired);
	emulate_irq_lapic *lapic = container(emulate_irq_lapic, event, lapic_qtimer);
	const vm_cpu *vm_cpu_timer = timer_expired->queueing_guest;

	// printf("EmLapicT IRQ %x\r\n", clock_read());
	deliver_irq_to_vm(vm_cpu_timer->vm, (lapic->lvt[0] & 0xff), 0);

	if (lapic->lvt[0] & 0x20000) {
		lapic->timer_current = lapic->timer_initial;
		lapic->timer_reference_value = clock_read();

		lapic_timer_reconfigure(lapic);
	}
}

// --------------------------------------------------------------------------

void emulate_irq_ioapic_store(const vm_cpu *vm_cpu_em, emulate_irq_lapic *lapic,
		uintptr_t offset, uint32_t value) {

	(void)vm_cpu_em;

	// printf("IOAPIC Store: %x = %x\r\n", offset, value);

	if (offset == 0x00) {
		lapic->ioregsel = value;
	} else if (offset == 0x10) {
		switch (lapic->ioregsel) {
		case 0x10 ... 0x3f:	/* IOREDTBL */
			lapic->ioredtbl[lapic->ioregsel - 0x10] = value;
		}
	}
}

uint32_t emulate_irq_ioapic_load(const vm_cpu *vm_cpu_em, emulate_irq_lapic *lapic,
		uintptr_t offset) {

	(void)vm_cpu_em;

	// printf("IOAPIC Load: %x\r\n", offset);

	if (offset == 0x00) {
		return lapic->ioregsel;
	} else if (offset == 0x10) {
		switch (lapic->ioregsel) {
		case 0x00:		/* IOAPIC ID */
			return 2 << 24;
		case 0x01:		/* IOAPIC Version */
			return 0x00170011;
		case 0x02:		/* IOAPIC ArbID */
			return 0;
		case 0x10 ... 0x3f:	/* IOREDTBL */
			return lapic->ioredtbl[lapic->ioregsel - 0x10];
		}
	}

	return 0xfffffff;
}

// --------------------------------------------------------------------------

void emulate_irq_lapic_store(const vm_cpu *vm_cpu_em, emulate_irq_lapic *lapic,
		uint32_t bar, uintptr_t offset, uint32_t value) {

	if (bar == 1) {
		emulate_irq_ioapic_store(vm_cpu_em, lapic, offset, value);
		return;
	}

	// printf("LAPIC Store: %x = %x\r\n", offset, value);

	switch (offset) {
	case 0xb0:		/* EOI */
		value = emulate_irq_lapic_activate_highest_interrupt(vm_cpu_em, lapic);
		// printf("EmEOI %x\r\n", value);
		lapic->irr[value >> 5] &= ~(1U << (value & 0x1f));
		return;
	case 0xd0:		/* LDR */
		lapic->destination = value;
		return;
	case 0xe0:		/* DFR */
		lapic->destination_mode = value;
		return;
	case 0x320 ... 0x370:	/* LVTs: {Timer, Thermal, PerfCounter, LInt0, LInt1, Error} */
		lapic->lvt[(offset - 0x320) >> 4] = value;
		if (offset == 0x320) {
			lapic_timer_reconfigure(lapic);
		}
		return;
	case 0x380:		/* Timer Initial Count */
		lapic->timer_initial = \
		lapic->timer_current = value;
		lapic->timer_reference_value = clock_read();
		lapic_timer_reconfigure(lapic);
		return;
	case 0x390:		/* Timer Current Count */
		lapic->timer_current = value;
		lapic->timer_reference_value = clock_read();
		lapic_timer_reconfigure(lapic);
		return;
	case 0x3e0:		/* Timer Divide */
		lapic->timer_divider = value;
		lapic->eff_timer_divisor = ((((value & 8) >> 1) | (value & 3)) + 1) % 8;
		return;
	case 0x3f0:		/* APIC Frequency */
		lapic->frequency = value;
		return;
	default:
		break;
	}
}

uint32_t emulate_irq_lapic_load(const vm_cpu *vm_cpu_em, emulate_irq_lapic *lapic,
		uint32_t bar, uintptr_t offset) {

	if (bar == 1) {
		return emulate_irq_ioapic_load(vm_cpu_em, lapic, offset);
	}

	// printf("LAPIC Load: %x\r\n", offset);

	switch (offset) {
	case 0x020:		/* APIC ID */
		return (vm_cpu_em->physical_cpu << 24);
	case 0x030:		/* APIC version */
		return 0x50010;
	case 0x0d0:		/* LDR */
		return lapic->destination;
	case 0x0e0:		/* DFR */
		return lapic->destination_mode;
	case 0x100 ... 0x170:	/* ISRs */
		return 0;
	case 0x200 ... 0x270:	/* IRRs */
		return 0;
	case 0x300:		/* ICR (low) */
		return 0;
	case 0x320 ... 0x370:	/* LVTs: {Timer, Thermal, PerfCounter, LInt0, LInt1, Error} */
		return lapic->lvt[(offset - 0x320) >> 4];
	case 0x380:		/* Timer Initial Count */
		return lapic->timer_initial;
	case 0x390:		/* Timer Current Count */
		lapic_estimate_current(lapic);
		return lapic->timer_current;
	case 0x3e0:		/* Timer Divide */
		return lapic->timer_divider;
	case 0x3f0:		/* APIC Frequency */
		return lapic->frequency;
	default:
		// spurious load
		return 0xffffffff;
	}
}

// --------------------------------------------------------------------------

void emulate_irq_lapic_initialize(const vm_cpu *vm_cpu_em, emulate_irq_lapic *lapic) {
	uint32_t i;

	(void)vm_cpu_em;

	for (i = 0; i < 6; i++)
		lapic->lvt[i] = LAPICLVT_MASK;

	for (i = 0; i < 24; i++) {
		lapic->ioredtbl[i << 1] = 0x10000; 	// IOAPIC_MASK
	}

	lapic->event.item.queueing_driver = TIMERDRIVER_EMULATE_LAPIC;
	lapic->event.item.queueing_subunit = 0;
	lapic->event.item.queueing_guest = vm_cpu_em;
	lapic->event.item.queueing_device = lapic;
}

uint32_t emulate_irq_lapic_activate_highest_interrupt(const struct vm_cpu_ *vm_cpu_em, emulate_irq_lapic *lapic) {
	uint32_t i, j;

	(void)vm_cpu_em;

	if (lapic->lvt[4] & LAPICLVT_MASK)
		return NO_PENDING_INTERRUPT;

	for (i = 0; i < 8; i++) {
		if (lapic->irr[i]) {
			asm volatile("bsf %1, %0" : "=r" (j) : "r" (lapic->irr[i]));
			return (i << 5) | j;
		}
	}

	return NO_PENDING_INTERRUPT;
}

void emulate_irq_lapic_set_interrupt_pending(const struct vm_cpu_ *vm_cpu_em, emulate_irq_lapic *lapic, uint32_t interrupt) {
	(void)vm_cpu_em;

	lapic->irr[interrupt >> 5] |= 1 << (interrupt & 0x1f);
}
