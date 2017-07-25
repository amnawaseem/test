#include <phidias.h>
#include <timequeue.h>
#include <mmio.h>
#include <drivers/clock.h>
#include <drivers/timer-pic32.h>
#include <arch/asm/offsets.h>
#include <drivers/irq.h>
#include <schedule.h>

void timer_pic32_setup(const memarea *area) {
	/* clear both control regs */
	mmio_write32(area->vaddr + PIC32REG_T2CON + CLR_OFFSET, 0xFFFF);
	mmio_write32(area->vaddr + PIC32REG_T3CON + CLR_OFFSET, 0xFFFF);

	/* enable the corresponding interrupt */
	irq_enable_irq(PIC32IRQ_TIMER3, 0x01e, &_timer_interrupt);

	/* combine T2 and T3 */
	mmio_write32(area->vaddr + PIC32REG_T2CON + SET_OFFSET, PIC32BIT_T32);
}

void timer_pic32_program(const memarea *area, timer *event) {
	uint64_t deadline = event->expiration - clock_read();
	uint32_t prescaler = 1;

	while((deadline >> 32) > 0) {
		prescaler <<= 1;
		deadline >>= 1;
		if (prescaler == 256)
			break;
	}

	/* not available on pic32 */
	if (prescaler == 128){
		prescaler <<= 1;
		deadline >>= 1;
	}

	/* convert into config bits */
	prescaler = timer_pic32_get_prescaler(prescaler);

	/* clear the prescaler */
	mmio_write32(area->vaddr + PIC32REG_T2CON + CLR_OFFSET, (0x7 << 4) | PIC32BIT_TON);

	/* set prescaler */
	mmio_write32(area->vaddr + PIC32REG_T2CON + SET_OFFSET, (prescaler << 4) | PIC32BIT_T32);

	/* set PR2 */
	mmio_write32(area->vaddr + PIC32REG_TMR2, 0);
	mmio_write32(area->vaddr + PIC32REG_PR2, deadline);

	/* finally, restart the timer */
	mmio_write32(area->vaddr + PIC32REG_T2CON + SET_OFFSET, PIC32BIT_TON);
}

void timer_pic32_interrupt (void){
	timequeue_interrupt();
	irq_ack_irq(PIC32IRQ_TIMER3);
	upcall();
}
