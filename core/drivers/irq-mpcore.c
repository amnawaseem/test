#include <phidias.h>
#include <mmio.h>
#include <drivers/irq-mpcore.h>
#include <asm/gic.h>

void irq_mpcore_setup(const memarea *area) {
	uint32_t i;

	mmio_write32(area->vaddr + MPCORE_IRQDIST_BASE + GIC_DIST_CTRL,
				GIC_DIST_CTRL_ENABLE0 | GIC_DIST_CTRL_ENABLE1);

	mmio_write32(area->vaddr + MPCORE_IRQCPU_BASE + GIC_CPU_CTRL,
				GIC_CPU_CTRL_ENABLE0 | GIC_CPU_CTRL_ENABLE1);
	mmio_write32(area->vaddr + MPCORE_IRQCPU_BASE + GIC_CPU_PMR,
				0xff);

	for (i = 0; i < 8; i++) {
		mmio_write32(area->vaddr + MPCORE_IRQDIST_BASE + GIC_DIST_CLEARPENDING_BASE + (i << 2),
				0xffffffff);
		mmio_write32(area->vaddr + MPCORE_IRQDIST_BASE + GIC_DIST_ENABLE_BASE + (i << 2),
				0xffffffff);
	}
}

uint32_t irq_mpcore_get_irq_raw(const memarea *area) {
	return mmio_read32(area->vaddr + MPCORE_IRQCPU_BASE + GIC_CPU_IAR);
}

void irq_mpcore_ack_irq(const memarea *area, uint32_t interrupt_number) {
	mmio_write32(area->vaddr + MPCORE_IRQCPU_BASE + GIC_CPU_EOIR,
				interrupt_number);
}

void irq_mpcore_disable_irq(const memarea *area, uint32_t interrupt_number) {
	uint32_t register_no = (interrupt_number >> 5);
	uint32_t bit_position = (interrupt_number & 0x1f);

	mmio_write32(area->vaddr + MPCORE_IRQDIST_BASE +
				GIC_DIST_DISABLE_BASE + (register_no << 2),
				(1 << bit_position));
}
