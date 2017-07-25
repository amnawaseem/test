#include <phidias.h>
#include <vm.h>
#include <drivers/clock.h>
#include <emulate/core.h>
#include <emulate/timer.h>
#include <emulate/irq.h>
#include <asm/mpcore.h>

static void mpcore_timer_reprogrammed(emulate_timer_mpcore *mpcore) {
	uint64_t interval;

	if (mpcore->queue_item.item.armed) {
		printf("Cannot reprogram armed MPCore timer, ignoring new value.\r\n");
		return;
	}

	interval = mpcore->value * (MPCORE_PRIVATETIMER_CONTROL_GET_PRESCALE(mpcore->control) + 1);
	mpcore->queue_item.item.expiration = clock_read() + interval;
	timequeue_insert(&mpcore->queue_item);
}

// --------------------------------------------------------------------------

void emulate_timer_mpcore_expired(timer *timer_expired) {
	const vm_cpu *vm_cpu_timer = timer_expired->queueing_guest;
	emulate_timer_mpcore *mpcore = timer_expired->queueing_device;

	// printf("MPCore timer expired on %x\r\n", vm_cpu_timer);

	emulate_irq_set_interrupt_pending(vm_cpu_timer, mpcore->interrupt_line);
	mpcore->interrupt_asserted = 1;
}

void emulate_timer_mpcore_initialize(const vm_cpu *vm_cpu_reset, emulate_timer_mpcore *mpcore) {
	mpcore->interrupt_line = 0x1d;
	mpcore->interrupt_asserted = 0;

	mpcore->queue_item.item.queueing_driver = TIMERDRIVER_EMULATE_MPCORE;
	mpcore->queue_item.item.queueing_guest = vm_cpu_reset;
	mpcore->queue_item.item.queueing_device = mpcore;
}

void emulate_timer_mpcore_store(const vm_cpu *vm_cpu_em, emulate_timer_mpcore *mpcore,
		uint32_t bar, uintptr_t offset, uint32_t value) {
	(void)vm_cpu_em; (void)bar;

	switch (offset + MPCORE_PRIVATETIMER_BASE) {
	case MPCORE_PRIVATETIMER_LOAD:
		mpcore->value = value;
		printf("MPCore Timer: %x\r\n", value);
		mpcore->value_set_at = clock_read();
		if (mpcore->control & MPCORE_PRIVATETIMER_CONTROL_ENABLE) {
			mpcore_timer_reprogrammed(mpcore);
		}
		break;
	case MPCORE_PRIVATETIMER_CONTROL:
		mpcore->control = value;
		if (mpcore->control & MPCORE_PRIVATETIMER_CONTROL_ENABLE) {
			mpcore_timer_reprogrammed(mpcore);
		}
		break;
	case MPCORE_PRIVATETIMER_IRQ_STATUS:
		if (mpcore->interrupt_asserted && (value & 1)) {
			mpcore->interrupt_asserted = 0;
			if (mpcore->control & MPCORE_PRIVATETIMER_CONTROL_AUTOLOAD) {
				mpcore->value_set_at = clock_read();
				mpcore_timer_reprogrammed(mpcore);
			}
		}
		break;
	default:
		printf("MPCore Unk W\r\n");
	}
}

uint32_t emulate_timer_mpcore_load(const vm_cpu *vm_cpu_em, emulate_timer_mpcore *mpcore,
		uint32_t bar, uintptr_t offset) {
	(void)vm_cpu_em; (void)bar;

	switch (offset + MPCORE_PRIVATETIMER_BASE) {
	case MPCORE_PRIVATETIMER_LOAD:
		return mpcore->value - (clock_read() - mpcore->value_set_at);
	case MPCORE_PRIVATETIMER_CONTROL:
		return mpcore->control;
	case MPCORE_PRIVATETIMER_IRQ_STATUS:
		return mpcore->interrupt_asserted;
	default:
		printf("MPCore Unk R\r\n");
		return 0;
	}
}
