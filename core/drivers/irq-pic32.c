#include <phidias.h>
#include <mmio.h>
#include <interrupts.h>
#include <drivers/irq-pic32.h>
#include <arch/asm/offsets.h>
#include <arch/cpu_ops.h>

extern char __phidias_tlb_handlers_start;

void irq_pic32_setup(const memarea *area) {
	/* setup:
	 * - set EBASE to location of handlers
	 * - use multi vector mode
	 */
	asm volatile("di");
	mtc0(c0_ebase, &__phidias_tlb_handlers_start);
	mmio_write32(area->vaddr + PIC32REG_INTCON + SET_OFFSET, PIC32BIT_MVEC);

	/* set shadow set 1 as set for INTs with prio 7 (hosts INTs) */
	mmio_write32(area->vaddr + PIC32REG_PRISS, 0x10000000);

	asm volatile("ei");
}

uint32_t irq_pic32_get_irq_raw(const memarea *area) {
	/* retrieve IRQ number */
	uint32_t irq_raw = mmio_read32(area->vaddr + PIC32REG_INTSTAT);
	return (irq_raw & 0xff);
}

void irq_pic32_ack_irq(const memarea *area, uint32_t raw_interrupt_number) {
	/* mark IRQ as acked: reset its flag */
	uint32_t div, mod;

	if (raw_interrupt_number > 228){
		printf("unreasonable interrupt number (ack): %d", raw_interrupt_number);
		return;
	}

	div = raw_interrupt_number / 32;
	mod = raw_interrupt_number % 32;

	mmio_write32(area->vaddr + 0x40 + (0x10 * div) + CLR_OFFSET, 1 << mod);
}

void irq_pic32_disable_irq(const memarea *area, uint32_t interrupt_number) {
	/* disable this IRQ: clear it's enable bit */
	uint32_t div, mod;

	if (interrupt_number > 228){
		printf("unreasonable interrupt number (disable): %d", interrupt_number);
		return;
	}

	div = interrupt_number / 32;
	mod = interrupt_number % 32;

	mmio_write32(area->vaddr + 0xC0 + (0x10 * div) + CLR_OFFSET, 1 << mod);
}

void irq_pic32_enable_irq(const memarea *area, uint32_t interrupt_number, uint8_t interrupt_priority, char *isr_ptr) {
	/* enable this IRQ: set it's enable bit, set prio, register service routine */
	if (interrupt_number > 213){
		printf("unreasonable interrupt id: %d\r\n", interrupt_number);
		return;
	}

	uint32_t mod, div, prio_mod, prio_div;

	mod = interrupt_number % 32;
	div = interrupt_number / 32;
	prio_mod = interrupt_number % 4;
	prio_div = interrupt_number / 4;

	asm volatile ("di");

	/* first, register the ISRs offset */
	int32_t offset = isr_ptr - &__phidias_tlb_handlers_start;
	mmio_write32(area->vaddr + 0x540 + (4*interrupt_number), offset);

	/* set the IRQs prio */
	mmio_write32(area->vaddr + 0x140 + (16*prio_div) + CLR_OFFSET, (0x1f << (prio_mod * 8)));
	mmio_write32(area->vaddr + 0x140 + (16*prio_div) + SET_OFFSET, (interrupt_priority << (prio_mod * 8)));

	/* set the IRQs interrupt enable bit */
	mmio_write32(area->vaddr + 0xc0 + (16*div) + SET_OFFSET, 1 << mod);

	asm volatile ("ei");
}

void irq_pic32_enable_irq_vm(const memarea *area, uint32_t irq_id, uint8_t prio, uint32_t offset, uint32_t guest_id){
	if (irq_id > 213){
		printf("unreasonable interrupt id: %d\r\n", irq_id);
		return;
	}

	uint32_t mod, div, prio_mod, prio_div;

	mod = irq_id % 32;
	div = irq_id / 32;
	prio_mod = irq_id % 4;
	prio_div = irq_id / 4;

	asm volatile ("di");

	/* first, register the ISRs offset */
	setGuestID(guest_id);
	mmio_write32(area->vaddr + 0x540 + (4*irq_id), offset);

	/* set the IRQs prio */
	mmio_write32(area->vaddr + 0x140 + (16*prio_div) + CLR_OFFSET, (0x1f << (prio_mod * 8)));
	mmio_write32(area->vaddr + 0x140 + (16*prio_div) + SET_OFFSET, (prio << (prio_mod * 8)));

	/* set the IRQs interrupt enable bit */
	mmio_write32(area->vaddr + 0xc0 + (16*div) + SET_OFFSET, 1 << mod);
	setGuestID(0);

	asm volatile ("ei");
}
