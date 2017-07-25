#include <phidias.h>
#include <vm.h>
#include <interrupts.h>
#include <drivers/clock.h>
#include <emulate/core.h>
#include <emulate/timer.h>
#include <emulate/irq.h>
#include <asm/sp804.h>
#include <trace.h>

static const uint8_t sp804_id[8] = {
	0x04, 0x18, 0x14, 0x00,
	0x0d, 0xf0, 0x05, 0xb1
};

static void sp804_timer_reprogrammed(emulate_timer_sp804 *sp804, uint32_t instance_index) {
	uint64_t interval;

	if (sp804->instances[instance_index].queue_item.item.armed) {
		printf("Cannot reprogram armed SP804 timer, ignoring new value.\r\n");
		return;
	}

	interval = sp804->instances[instance_index].value;
	interval <<= SP804_TIMER_CONTROL_PRESCALE_SHIFT(sp804->instances[instance_index].control);

	// shift from 19.2Mhz expected SP804 frequency to 1.2MHz CP14 frequency
	interval >>= 4;

	sp804->instances[instance_index].queue_item.item.expiration = clock_read() + interval;
	timequeue_insert(&sp804->instances[instance_index].queue_item);
}

static uint32_t sp804_queuetimer_to_index(emulate_timer_sp804 *sp804, queue_timer *qt) {
	if (qt == &sp804->instances[0].queue_item)
		return 0;
	else
		return 1;
}

// --------------------------------------------------------------------------

void emulate_timer_sp804_expired(timer *timer_expired) {
	const vm_cpu *vm_cpu_timer = timer_expired->queueing_guest;
	emulate_timer_sp804 *sp804 = timer_expired->queueing_device;
	emulate_timer_sp804_instance *sp804i;
	queue_timer *queuetimer_expired = container(queue_timer, item, timer_expired);

	uint32_t instance_idx = sp804_queuetimer_to_index(sp804, queuetimer_expired);
	sp804i = &sp804->instances[instance_idx];

	if (sp804i->control & SP804_TIMER_CONTROL_IRQENABLE) {
		//printf("SP804 timer[%d] expired on %x\r\n", instance_idx, vm_cpu_timer);
		deliver_irq_to_vm(vm_cpu_timer->vm, sp804->interrupt_line, 1);
		sp804->interrupt_asserted = 1;
	}

	if (sp804i->control & SP804_TIMER_CONTROL_PERIODIC) {
		sp804i->value = sp804i->load;
		sp804i->value_set_at = clock_read();
		sp804_timer_reprogrammed(sp804, instance_idx);
	}
}

void emulate_timer_sp804_initialize(const vm_cpu *vm_cpu_reset, emulate_timer_sp804 *sp804) {
	sp804->interrupt_line = 0x2e;
	sp804->interrupt_asserted = 0;

	sp804->instances[0].queue_item.item.queueing_driver = TIMERDRIVER_EMULATE_SP804;
	sp804->instances[0].queue_item.item.queueing_guest = vm_cpu_reset;
	sp804->instances[0].queue_item.item.queueing_device = sp804;

	sp804->instances[1].queue_item.item.queueing_driver = TIMERDRIVER_EMULATE_SP804;
	sp804->instances[1].queue_item.item.queueing_guest = vm_cpu_reset;
	sp804->instances[1].queue_item.item.queueing_device = sp804;
}

void emulate_timer_sp804_store(const vm_cpu *vm_cpu_em, emulate_timer_sp804 *sp804,
		uint32_t bar, uintptr_t offset, uint32_t value) {
	(void)vm_cpu_em;

	// reuse bar to identify SP804 instance
	if (offset < 2*SP804_INSTANCE_OFFSET) {
		bar = (offset & SP804_INSTANCE_OFFSET) ? 1 : 0;
		offset &= (SP804_INSTANCE_OFFSET) - 1;
	}

	switch (offset) {
	case SP804_TIMER_LOAD:
		sp804->instances[bar].load = value;
		sp804->instances[bar].value = value;
		//printf("SP804 Timer[%d]: %x\r\n", bar, value);
		sp804->instances[bar].value_set_at = clock_read();
		if (sp804->instances[bar].control & SP804_TIMER_CONTROL_ENABLE) {
			sp804_timer_reprogrammed(sp804, bar);
		}
		break;
	case SP804_TIMER_VALUE:	/* illegal according to specification */
		break;
	case SP804_TIMER_CONTROL:
		sp804->instances[bar].control = value;
		if (SP804_TIMER_CONTROL_PRESCALE_SHIFT(sp804->instances[bar].control) == 0xc) {
			printf("Invalid SP804 scaling!\r\n");
		}
		if (sp804->instances[bar].control & SP804_TIMER_CONTROL_ENABLE) {
			sp804_timer_reprogrammed(sp804, bar);
		}
		break;
	case SP804_TIMER_IRQCLEAR:
		sp804->interrupt_asserted = 0;
		break;
	case SP804_TIMER_BGLOAD:
		sp804->instances[bar].load = value;
		/*printf("SP804 Timer[%d]: %x (BG %x)\r\n", bar,
			sp804->instances[bar].value, value);*/
		break;
	default:
		printf("SP804 Unk W %x\r\n", offset);
	}
}

uint32_t emulate_timer_sp804_load(const vm_cpu *vm_cpu_em, emulate_timer_sp804 *sp804,
		uint32_t bar, uintptr_t offset) {
	uint64_t timeout, realdelta;

	(void)vm_cpu_em;

	// reuse bar to identify SP804 instance
	if (offset < 2*SP804_INSTANCE_OFFSET) {
		bar = (offset & SP804_INSTANCE_OFFSET) ? 1 : 0;
		offset &= (SP804_INSTANCE_OFFSET) - 1;
	}

	switch (offset) {
	case SP804_TIMER_LOAD:
		return sp804->instances[bar].load;
	case SP804_TIMER_VALUE:
		timeout = sp804->instances[bar].value;
		timeout <<= SP804_TIMER_CONTROL_PRESCALE_SHIFT(sp804->instances[bar].control);
		realdelta = (clock_read() - sp804->instances[bar].value_set_at) >> 4;

		return timeout - realdelta;
	case SP804_TIMER_CONTROL:
		return sp804->instances[bar].control;
	case SP804_TIMER_RAWIRQSTATUS:
		return sp804->interrupt_asserted;
	case SP804_TIMER_MASKEDIRQSTATUS:
		return sp804->interrupt_asserted &&
			((sp804->instances[0].control | sp804->instances[1].control) &
				SP804_TIMER_CONTROL_IRQENABLE) ? 1 : 0;
	case SP804_TIMER_BGLOAD:
		return sp804->instances[bar].load;
	case AMBA_PERIPHID0 ... AMBA_PRIMECELLID3:
		return sp804_id[(offset - AMBA_PERIPHID0) >> 2];
	default:
		printf("SP804 Unk R %x\r\n", offset);
		return 0;
	}
}
