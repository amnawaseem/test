#include <phidias.h>
#include <mmio.h>
#include <memarea.h>
#include <interrupts.h>
#include <drivers/irq-gic.h>

static void irq_gic_setup_global(const memarea *area) {
	uint32_t i;

	for (i = 32/1; i < 256/1; i += 4) {
		mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
				GIC_DIST_TARGETS_BASE + i, 0x01010101);
		mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
				GIC_DIST_PRIORITY_BASE + i, 0x90909090);
	}

	for (i = 32/8; i < 256/8; i += 4) {
		mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
				GIC_DIST_CLEARPENDING_BASE + i, 0xffffffff);
		mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
				GIC_DIST_ENABLE_BASE + i, 0xffffffff);
	}
}

void irq_gic_setup(const memarea *area) {
	uint32_t i;

	mmio_write32(area->vaddr + GICV2_AREAOFFSET_CPU + GIC_CPU_PMR,
			0xff);

	if (cpu_number == 0) {
		irq_gic_setup_global(area);
	}

	for (i = 0/1; i < 32/1; i += 4) {
		mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
				GIC_DIST_PRIORITY_BASE + i, 0x90909090);
	}

	mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
			GIC_DIST_CLEARPENDING_BASE, 0xffffffff);
	mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
			GIC_DIST_ENABLE_BASE, 0xffffffff);

	mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST + GIC_DIST_CTRL,
			GIC_DIST_CTRL_ENABLE0 | GIC_DIST_CTRL_ENABLE1);
	mmio_write32(area->vaddr + GICV2_AREAOFFSET_CPU + GIC_CPU_CTRL,
			GIC_CPU_CTRL_ENABLE0 | GIC_CPU_CTRL_ENABLE1 | GIC_CPU_CTRL_EOIMODE);
}

uint32_t irq_gic_get_irq_raw(const memarea *area) {
	uint32_t raw_irq;

	raw_irq = mmio_read32(area->vaddr + GICV2_AREAOFFSET_CPU + GIC_CPU_IAR);

	if (raw_irq == GICIRQ_SPURIOUS) {
		return IRQID_SPURIOUS;
	}

	// directly write EOI as well; interrupt is disabled separately
	mmio_write32(area->vaddr + GICV2_AREAOFFSET_CPU + GIC_CPU_EOIR,
			raw_irq);

	return raw_irq;
}

void irq_gic_ack_irq(const memarea *area, uint32_t raw_interrupt_number) {
	mmio_write32(area->vaddr + GICV2_AREAOFFSET_CPU + GIC_CPU_DIR,
			raw_interrupt_number);
}

void irq_gic_disable_irq(const memarea *area, uint32_t interrupt_number) {
	uint32_t register_no = (interrupt_number >> 5);
	uint32_t bit_position = (interrupt_number & 0x1f);

	mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST +
				GIC_DIST_DISABLE_BASE + (register_no << 2),
				(1 << bit_position));
}

void irq_gic_raise_ipi(const memarea *area, uint32_t target_cpu_number) {
	mmio_write32(area->vaddr + GICV2_AREAOFFSET_DIST + GIC_DIST_SGIR,
				((1U << (16+target_cpu_number)) & 0xff0000) | 4);
}
