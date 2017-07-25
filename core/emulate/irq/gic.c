#include <phidias.h>
#include <vm.h>
#include <lock.h>
#include <mmio.h>
#include <memarea.h>
#include <xcore.h>
#include <emulate/core.h>
#include <emulate/irq.h>
#include <asm/gic.h>
#include <arch/fpu_support.h>

static void gic_cpu_store(const vm_cpu *vm_cpu_em, emulate_irq_gic *gic,
		uintptr_t offset, uint32_t value) {
	(void)vm_cpu_em;

	switch (offset) {
	case GIC_CPU_CTRL:
		gic->control = value;
		break;
	case GIC_CPU_PMR:
		gic->pmr = value;
		break;
	case GIC_CPU_BPR:
		gic->bpr = value;
		break;
	case GIC_CPU_EOIR:
		// printf("GICC EOI %x\r\n", value);
		// TODO: check if this was a HW passthrough IRQ -> ack at HW controller
		break;
	default:
		printf("Unknown GICC store (o %x)\r\n", offset);
		break;
	}
}

static uint32_t gic_cpu_load(const vm_cpu *vm_cpu_em, emulate_irq_gic *gic,
		uintptr_t offset) {
	uint32_t tmp;

	(void)vm_cpu_em;

	switch (offset) {
	case GIC_CPU_CTRL:
		return gic->control;
	case GIC_CPU_PMR:
		return gic->pmr;
	case GIC_CPU_BPR:
		return gic->bpr;
	case GIC_CPU_IAR:
		tmp = gic->active_irq;
		// printf("GICC ACK %x\r\n", tmp);
		gic->active_irq = GICIRQ_SPURIOUS;
		return tmp;
	default:
		printf("Unknown GICC load (o %x)\r\n", offset);
		return 0;
	}
}

// --------------------------------------------------------------------------

static void gic_distributor_store(const vm_cpu *vm_cpu_em, emulate_irq_gic *gic,
		uintptr_t offset, uint32_t value) {
	(void)vm_cpu_em;

	switch (offset) {
	case GIC_DIST_CTRL:
		gic->distributor->control = value;
		break;
	case GIC_DIST_ENABLE_BASE:
		printf("GICD En: %x %x\r\n", offset, value);
		gic->private_masked_bits[0] &= ~value;
		break;
	case (GIC_DIST_ENABLE_BASE + 4) ... (GIC_DIST_DISABLE_BASE - 4):
		printf("GICD En: %x %x\r\n", offset, value);
		gic->distributor->masked_bits[(offset - GIC_DIST_ENABLE_BASE - 4) >> 2] &= ~value;
		break;
	case GIC_DIST_DISABLE_BASE:
		gic->private_masked_bits[0] |= value;
		break;
	case (GIC_DIST_DISABLE_BASE + 4) ... (GIC_DIST_SETPENDING_BASE - 4):
		gic->distributor->masked_bits[(offset - GIC_DIST_DISABLE_BASE - 4) >> 2] |= value;
		break;
	case GIC_DIST_SETACTIVE_BASE:
		gic->private_pending_bits[0] |= value;
		break;
	case (GIC_DIST_SETACTIVE_BASE + 4) ... (GIC_DIST_CLEARACTIVE_BASE - 4):
		gic->distributor->pending_bits[(offset - GIC_DIST_SETACTIVE_BASE - 4) >> 2] |= value;
		break;
	case GIC_DIST_CLEARACTIVE_BASE:
		gic->private_pending_bits[0] &= ~value;
		break;
	case (GIC_DIST_CLEARACTIVE_BASE + 4) ... (GIC_DIST_PRIORITY_BASE - 4):
		gic->distributor->pending_bits[(offset - GIC_DIST_CLEARACTIVE_BASE - 4) >> 2] &= ~value;
		break;
	case GIC_DIST_PRIORITY_BASE:
		gic->private_priorities[0] = value;
		break;
	case (GIC_DIST_PRIORITY_BASE + 4) ... (GIC_DIST_TARGETS_BASE - 4):
		gic->distributor->priorities[(offset - GIC_DIST_PRIORITY_BASE - 4) >> 2] = value;
		break;
	case GIC_DIST_TARGETS_BASE:
		break;
	case (GIC_DIST_TARGETS_BASE + 4) ... (GIC_DIST_CONFIG_BASE - 4):
		gic->distributor->target_bits[(offset - GIC_DIST_TARGETS_BASE - 4) >> 2] = value;
		break;
	case GIC_DIST_CONFIG_BASE ... (GIC_DIST_NSACR_BASE - 4):
		break;
	case GIC_DIST_SGIR:
		if ((value & 0x03000000) == 0) {
			uint32_t from_vcpu = vm_cpu_em->vm->cpus - vm_cpu_em;
			uint32_t to_vcpu;
			for (to_vcpu = 0; to_vcpu < 8; to_vcpu++) {
				if (!(value & (1U << (16+to_vcpu))))
					continue;
				xcore_virtual_ipi(vm_cpu_em->vm, from_vcpu,
							to_vcpu, value & 0xff);
			}
		} else {
			printf("Unsupported GICD SGIR\r\n");
		}
		break;
	default:
		printf("Unknown GICD store (o %x) %x\r\n", offset, value);
		break;
	}
}

static uint32_t gic_distributor_load(const vm_cpu *vm_cpu_em, emulate_irq_gic *gic,
		uintptr_t offset) {
	(void)vm_cpu_em;

	switch (offset) {
	case GIC_DIST_CTRL:
		return gic->distributor->control;
	case GIC_DIST_ENABLE_BASE:
		return ~gic->private_masked_bits[0];
	case (GIC_DIST_ENABLE_BASE + 4) ... (GIC_DIST_DISABLE_BASE - 4):
		return ~gic->distributor->masked_bits[(offset - GIC_DIST_ENABLE_BASE - 4) >> 2];
	case GIC_DIST_TYPE:
		return GIC_DIST_TYPE_CPUNO(0) | GIC_DIST_TYPE_NLINES(96);
	case GIC_DIST_SETACTIVE_BASE:
		return gic->private_pending_bits[0];
	case (GIC_DIST_SETACTIVE_BASE + 4) ... (GIC_DIST_CLEARACTIVE_BASE - 4):
		return gic->distributor->pending_bits[(offset - GIC_DIST_SETACTIVE_BASE - 4) >> 2];
	case GIC_DIST_PRIORITY_BASE:
		return gic->private_priorities[0];
	case (GIC_DIST_PRIORITY_BASE + 4) ... (GIC_DIST_TARGETS_BASE - 4):
		return gic->distributor->priorities[(offset - GIC_DIST_PRIORITY_BASE - 4) >> 2];
	case GIC_DIST_TARGETS_BASE:
		return 0x01010101;
	case (GIC_DIST_TARGETS_BASE + 4) ... (GIC_DIST_CONFIG_BASE - 4):
		return gic->distributor->target_bits[(offset - GIC_DIST_TARGETS_BASE - 4) >> 2];
	case GIC_DIST_CONFIG_BASE ... (GIC_DIST_NSACR_BASE - 4):
		return 0x55555555;
	default:
		printf("Unknown GICD load (o %x)\r\n", offset);
		return 0;
	}
}

// --------------------------------------------------------------------------

void emulate_irq_gic_store(const vm_cpu *vm_cpu_em, emulate_irq_gic *gic,
		uint32_t bar, uintptr_t offset, uint32_t value) {
	if (offset & 3) {
		printf("Unaligned GIC store, ignored\r\n");
		return;
	}

	if (bar == 0) {
		// printf("GIC D W %x <- %x\r\n", offset, value);
		spinlock_lock(&gic->distributor->lock);
		gic_distributor_store(vm_cpu_em, gic, offset, value);
		spinlock_unlock(&gic->distributor->lock);
	} else {
		gic_cpu_store(vm_cpu_em, gic, offset, value);
	}
}

uint32_t emulate_irq_gic_load(const vm_cpu *vm_cpu_em, emulate_irq_gic *gic,
		uint32_t bar, uintptr_t offset) {
	if (offset & 3) {
		printf("Unaligned GIC load, returning 0\r\n");
		return 0;
	}

	if (bar == 0) {
		uint32_t value;
		spinlock_lock(&gic->distributor->lock);
		value = gic_distributor_load(vm_cpu_em, gic, offset);
		spinlock_unlock(&gic->distributor->lock);
		// printf("GIC D R %x -> %x\r\n", offset, value);
		return value;
	} else {
		return gic_cpu_load(vm_cpu_em, gic, offset);
	}
}

void emulate_irq_gic_initialize(const vm_cpu *vm_cpu_em, emulate_irq_gic *gic) {
	uint32_t i;

	(void)vm_cpu_em;

	gic->active_irq = GICIRQ_SPURIOUS;
	gic->distributor->lock = 0;
	gic->control = 0;
	gic->distributor->control = 0;
	gic->private_masked_bits[0] = 0xffffffff;

	for (i = 0; i < ((1024 - 32) >> 5); i++) {
		gic->distributor->masked_bits[i] = 0xffffffff;
	}

	gic->virt_listregs[0] = 0;
	gic->virt_listregs[1] = 0;
	gic->virt_listregs[2] = 0;
	gic->virt_listregs[3] = 0;
}

// --------------------------------------------------------------------------

static uint32_t emulate_irq_gic_pull_next_active(emulate_irq_gic *gic) {
	uint32_t next_active = GICIRQ_SPURIOUS;
	uint32_t pending_unmasked;
	uint32_t i;

	pending_unmasked = gic->private_pending_bits[0] & ~gic->private_masked_bits[0];
	if (pending_unmasked) {
		next_active = highest_bit(pending_unmasked);
		if (next_active < GICIRQ_PPI_BASE) {
			next_active |= (gic->sgi_source[next_active] << 10);
		}
		gic->private_pending_bits[0] &= ~(1U << (next_active & 0x1f));
		return next_active;
	}

	spinlock_lock(&gic->distributor->lock);
	for (i = 0; i < ((1024-32) >> 5); i++) {
		pending_unmasked = gic->distributor->pending_bits[i];
		if (pending_unmasked == 0) continue;
		pending_unmasked &= ~gic->distributor->masked_bits[i];
		if (pending_unmasked == 0) continue;
		next_active = (i << 5) + 32 + highest_bit(pending_unmasked);
		gic->distributor->pending_bits[i] &= ~(1U << (next_active & 0x1f));
		break;
	}
	spinlock_unlock(&gic->distributor->lock);

	return next_active;
}

uint32_t emulate_irq_gic_activate_highest_interrupt(const vm_cpu *vm_cpu_inj, emulate_irq_gic *gic) {
	(void)vm_cpu_inj;

	if ((gic->distributor->control & 1) == 0) {
		return NO_PENDING_INTERRUPT;
	}
	if ((gic->control & 1) == 0) {
		return NO_PENDING_INTERRUPT;
	}
	if (gic->active_irq != GICIRQ_SPURIOUS) {
		return gic->active_irq;
	}

	gic->active_irq = emulate_irq_gic_pull_next_active(gic);

	if (gic->active_irq == GICIRQ_SPURIOUS) {
		return NO_PENDING_INTERRUPT;
	} else {
		// printf("ActHigh %x\r\n", gic->active_irq);
		return gic->active_irq;
	}
}

void emulate_irq_gic_set_interrupt_pending(const vm_cpu *vm_cpu_inj,
		emulate_irq_gic *gic, uint32_t interrupt) {
	(void)vm_cpu_inj;

	if (interrupt < GICIRQ_SPI_BASE) {
		gic->private_pending_bits[0] |= (1U << interrupt);
	} else {
		spinlock_lock(&gic->distributor->lock);
		gic->distributor->pending_bits[(interrupt >> 5) - 1] |= (1U << (interrupt & 0x1f));
		spinlock_unlock(&gic->distributor->lock);
	}
}

// --------------------------------------------------------------------------
// Interface changes for the Virt.Ext. enhanced GIC:
// A) hardware state save/restore functions
// B) activate_highest() manages the list registers before upcall

uint32_t emulate_irq_gicve_activate_highest_interrupt(const vm_cpu *vm_cpu_inj, emulate_irq_gic *gic) {
	uintptr_t mmiobase = core_memarea(MEMAREA_IRQC)->vaddr + GICV2_AREAOFFSET_HYP;
	uint32_t i;
	uint32_t empty_lr_mask;

	(void)vm_cpu_inj;

	if (gic->active_irq == GICIRQ_SPURIOUS) {
		gic->active_irq = emulate_irq_gic_pull_next_active(gic);
	}
	if (gic->active_irq == GICIRQ_SPURIOUS) {
		return NO_PENDING_INTERRUPT;
	}

	empty_lr_mask = mmio_read32(mmiobase + GIC_HYP_ELSR(0));
	if (empty_lr_mask == 0) {
		return NO_PENDING_INTERRUPT;
	}

	// ok, we have something to inject and we have at least one LR
	for (i = 0; i < 4; i++) {
		if (!(empty_lr_mask & (1 << i)))
			continue;

		gic->virt_listregs[i] = 
			GIC_HYP_LR_HW | GIC_HYP_LR_STATE_PENDING |
			(0x18 << GIC_HYP_LR_PRIO__SHIFT) |
			(gic->active_irq << GIC_HYP_LR_PHYSID__SHIFT) |
			(gic->active_irq << GIC_HYP_LR_VIRTID__SHIFT);

		mmio_write32(mmiobase + GIC_HYP_LIST(i), gic->virt_listregs[i]);
		// printf("LRn set: %x\r\n", gic->virt_listregs[i]);

		gic->active_irq = GICIRQ_SPURIOUS;
		gic->active_irq = emulate_irq_gic_pull_next_active(gic);
		if (gic->active_irq == GICIRQ_SPURIOUS) {
			break;
		}
	}

	return NO_PENDING_INTERRUPT;
}

void emulate_irq_gicve_save_state(const vm_cpu *vm_cpu_inj, emulate_irq_gic *gic) {
	uint32_t i;
	uintptr_t mmiobase = core_memarea(MEMAREA_IRQC)->vaddr + GICV2_AREAOFFSET_HYP;

	(void)vm_cpu_inj;

	for (i = 0; i < 4; i++) {
		gic->virt_listregs[i] = mmio_read32(mmiobase + GIC_HYP_LIST(i));
		// printf("(Saving) LRn was set: %x\r\n", gic->virt_listregs[i]);
	}

	gic->virt_control = mmio_read32(mmiobase + GIC_HYP_VMCR);
	// printf("(Saving) VGIC state is: %x\r\n", gic->virt_control);

	mmio_write32(mmiobase + GIC_HYP_CTRL, 0);
}

void emulate_irq_gicve_load_state(const vm_cpu *vm_cpu_inj, emulate_irq_gic *gic) {
	uint32_t i;
	uintptr_t mmiobase = core_memarea(MEMAREA_IRQC)->vaddr + GICV2_AREAOFFSET_HYP;

	(void)vm_cpu_inj;

	for (i = 0; i < 4; i++) {
		mmio_write32(mmiobase + GIC_HYP_LIST(i), gic->virt_listregs[i]);
	}

	if ((mmio_read32(mmiobase + GIC_HYP_ELSR(0)) == 0) && (gic->active_irq != GICIRQ_SPURIOUS)) {
		mmio_write32(mmiobase + GIC_HYP_CTRL, GIC_HYP_CTRL_ENABLE | GIC_HYP_CTRL_UNDERFLOW);
	} else {
		mmio_write32(mmiobase + GIC_HYP_CTRL, GIC_HYP_CTRL_ENABLE);
	}

	mmio_write32(mmiobase + GIC_HYP_VMCR, gic->virt_control);
}
