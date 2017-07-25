#include <phidias.h>
#include <timequeue.h>
#include <mmio.h>
#include <drivers/clock.h>
#include <drivers/timer-mpcore.h>
#include <trace.h>

void timer_mpcore_setup(const memarea *area) {
	(void)area;
}

void timer_mpcore_program(const memarea *area, timer *event) {
	uint64_t time_now = clock_read();
	uint64_t deadline = event->expiration - time_now;
	uint32_t prescaler = 1;

	while ((deadline >> 32) > 0) {
		prescaler <<= 1;
		deadline >>= 1;
		if (prescaler == 256)
			break;
	}

	mmio_write32(area->vaddr + MPCORE_PRIVATETIMER_LOAD,
			(uint32_t)deadline);
	mmio_write32(area->vaddr + MPCORE_PRIVATETIMER_CONTROL,
			MPCORE_PRIVATETIMER_CONTROL_PRESCALE(prescaler-1) |
			MPCORE_PRIVATETIMER_CONTROL_ENABLE |
			MPCORE_PRIVATETIMER_CONTROL_IRQ_ENABLE);
	// trace("tp", (uint32_t)time_now, (uint32_t)deadline, (uint32_t)event->expiration, prescaler);
	// printf("Programmed! (QD %x/%x) (D %qx)\r\n", event->queueing_driver, event->queueing_subunit, event->expiration);
}

uint32_t timer_mpcore_interrupt(const memarea *area) {
	mmio_write32(area->vaddr + MPCORE_PRIVATETIMER_IRQ_STATUS, 1);
	// trace("tI", (uint32_t)clock_read(), 0, 0, 0);

	timequeue_interrupt();
	return 0;
}
