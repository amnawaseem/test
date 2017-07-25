#include <phidias.h>
#include <mmio.h>
#include <drivers/clock-pic32.h>
#include <arch/asm/offsets.h>
#include <drivers/irq.h>
#include <schedule.h>
#include <arch/cpu_ops.h>

uint32_t upper_half = 0;

void clock_pic32_setup(void) {
	/* reset count register */
	mtc0(c0_count, 0);

	/* set compare */
	mtc0(c0_compare, 0xFFFFFFFF);

	irq_enable_irq(PIC32IRQ_CORETIMER, 0x1f, &_clock_interrupt);
}

uint64_t clock_pic32_read(void) {
	uint32_t clock_low = mfc0(c0_count);
	uint64_t clock_value;
	clock_value = ((uint64_t)upper_half << 32) | clock_low;
	return clock_value;
}

void clock_pic32_interrupt (void){
	mtc0(c0_compare, 0xFFFFFFFF);
	upper_half++;
	irq_ack_irq(PIC32IRQ_CORETIMER);
	upcall();
}
