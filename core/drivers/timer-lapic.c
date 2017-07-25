#include <phidias.h>
#include <timequeue.h>
#include <mmio.h>
#include <drivers/clock.h>
#include <drivers/timer-lapic.h>
#include <drivers/irq-lapic.h>
#include <trace.h>

void timer_lapic_setup(const memarea *area) {
	mmio_write32(area->vaddr + LAPICREG_TIMER_DIVISOR, 0xb);
	mmio_write32(area->vaddr + LAPICREG_LVT_TIMER, IRQNO_LAPIC_TIMER | LAPICLVT_MASK);
}

void timer_lapic_program(const memarea *area, timer *event) {
	uint64_t time_now = clock_read();
	uint64_t deadline = event->expiration - time_now;

	// printf("Deadline: %x\r\n", deadline);
	if ((deadline >> 32) > 0) {
		deadline = 0xffffffffU;
	}

	mmio_write32(area->vaddr + LAPICREG_TIMER_DIVISOR, 0);
	mmio_write32(area->vaddr + LAPICREG_TIMER_INITIAL, (uint32_t)deadline);
	mmio_write32(area->vaddr + LAPICREG_LVT_TIMER, IRQNO_LAPIC_TIMER);

	// trace("tp", (uint32_t)time_now, (uint32_t)deadline, (uint32_t)event->expiration, prescaler);
	// printf("Programmed! (QD %x/%x) (D %qx)\r\n", event->queueing_driver, event->queueing_subunit, event->expiration);
}

uint32_t timer_lapic_interrupt(const memarea *area) {
	(void)area;

	irq_lapic_ack_irq(area, NULL, 0xdd);

	// trace("tI", (uint32_t)clock_read(), 0, 0, 0);
	timequeue_interrupt();

	return 0;
}
