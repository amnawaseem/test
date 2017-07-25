#include <phidias.h>
#include <mmio.h>
#include <drivers/irq-lapic.h>
#include <asm/gic.h>

#define	PICNO_MASTER_BASE	0x20
#define	PICNO_SLAVE_BASE	0x28

uint32_t _incoming_idt_vector = 0;

static void silence_pic(void) {
	uint32_t i;
	volatile uint32_t j;
	uint8_t piccmd[10] = { 0x11, 0x11, 0x20, 0x28,    4,    2, 0x02, 0x02, 0xef, 0xff, };
	uint16_t picreg[10] = { 0x20, 0xa0, 0x21, 0xa1, 0x21, 0xa1, 0x21, 0xa1, 0x21, 0xa1, };

	for (i = 0; i < 10; i++) {
		asm volatile("outb %0, (%1)" :: "a" (piccmd[i]), "d" (picreg[i]));
		for (j = 0x20000; j; j--) ;
	}
}

void irq_lapic_setup(const memarea *area_lapic, const memarea *area_ioapic) {
	uint32_t i;

	silence_pic();

	mmio_write32(area_lapic->vaddr + LAPICREG_SPURIOUS, 0x1ff);
	//mmio_write32(area_lapic->vaddr + LAPICREG_LVT_TIMER, 

	for (i = 0; i < IOAPIC_REDTBL_COUNT; i++) {
		mmio_write32(area_ioapic->vaddr + IOAPICREG_SEL, IOAPICREGWIN_IOREDTBL + (i*2));
		mmio_write32(area_ioapic->vaddr + IOAPICREG_DATA, 0x00000000);
		mmio_write32(area_ioapic->vaddr + IOAPICREG_SEL, IOAPICREGWIN_IOREDTBL + 1 + (i*2));
		mmio_write32(area_ioapic->vaddr + IOAPICREG_DATA, 0x0000a000 + IRQNO_IOAPIC_BASE + i);
	}
}

uint32_t irq_lapic_get_irq_raw(const memarea *area_lapic, const memarea *area_ioapic) {
	(void)area_lapic;
	(void)area_ioapic;

	return _incoming_idt_vector;
}

void irq_lapic_ack_irq(const memarea *area_lapic, const memarea *area_ioapic, uint32_t interrupt_number) {
	(void)area_ioapic;

	if ((interrupt_number >= IRQNO_IOAPIC_BASE) &&
		(interrupt_number < IRQNO_IOAPIC_BASE + IOAPIC_REDTBL_COUNT)) {
		mmio_write32(area_lapic->vaddr + LAPICREG_EOI, interrupt_number);
	} else if ((interrupt_number >= PICNO_MASTER_BASE) &&
		(interrupt_number < PICNO_SLAVE_BASE)) {
		asm volatile("outb %b0, (%w1)" :: "a" (0x20), "d" (0x20));
	} else if ((interrupt_number >= PICNO_SLAVE_BASE) &&
		(interrupt_number < IRQNO_IOAPIC_BASE)) {
		asm volatile("outb %b0, (%w1)" :: "a" (0x20), "d" (0xa0));
		asm volatile("outb %b0, (%w1)" :: "a" (0x20), "d" (0x20));
	} else if (interrupt_number == IRQNO_LAPIC_TIMER) {
		mmio_write32(area_lapic->vaddr + LAPICREG_EOI, interrupt_number);
	} else {
		printf("Lapic doesn't know how to ack 0x%x\r\n", interrupt_number);
	}
}

void irq_lapic_disable_irq(const memarea *area_lapic, const memarea *area_ioapic, uint32_t interrupt_number) {
	(void)area_lapic;
	(void)area_ioapic;

	printf("Lapic Disable 0x%x ignored\r\n", interrupt_number);
}

void irq_lapic_raise_ipi(const memarea *area_lapic, uint32_t target_cpu_number) {
	printf("Lapic IPI %d -> NOP\r\n", target_cpu_number);
}
