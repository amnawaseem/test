#include <phidias.h>
#include <emulate/core.h>
#include <emulate/timer.h>
#include <interrupts.h>
#include <drivers/clock.h>
#include <vm.h>
#include <misc_ops.h>
#include <schedule.h>

void emulate_timer_armcp14_initialize(const vm_cpu *vm_cpu_reset, emulate_timer_armcp14 *armcp14) {
	memset(armcp14, 0, sizeof(emulate_timer_armcp14));

	armcp14->queue_item_virt.item.queueing_driver = TIMERDRIVER_EMULATE_ARMCP14;
	armcp14->queue_item_virt.item.queueing_subunit = 0;
	armcp14->queue_item_virt.item.queueing_guest = vm_cpu_reset;
	armcp14->queue_item_virt.item.queueing_device = armcp14;

	armcp14->queue_item_phys.item.queueing_driver = TIMERDRIVER_EMULATE_ARMCP14;
	armcp14->queue_item_phys.item.queueing_subunit = 1;
	armcp14->queue_item_phys.item.queueing_guest = vm_cpu_reset;
	armcp14->queue_item_phys.item.queueing_device = armcp14;
}

uint32_t emulate_timer_armcp14_interrupt() {
	// find out whether the current VM has an appropriate vdev
	if (current_vm_cpu == NULL) {
		printf("Ouch, EL1 timer fired without current VCPU.\r\n");
		return IRQ_KEEP_MASKED;
	}

	deliver_irq_to_vm(current_vm_cpu->vm, 0x1b, 1);

	return IRQ_KEEP_MASKED;
}

void emulate_timer_armcp14_save_state(const vm_cpu *vm_cpu_em, emulate_timer_armcp14 *armcp14) {
	(void)vm_cpu_em;

	armcp14->vcval_read_at = clock_read();

	asm volatile("mrs %0, CNTV_CVAL_EL0" : "=r" (armcp14->cntv_cval));
	asm volatile("mrs %0, CNTV_CTL_EL0" : "=r" (armcp14->cntv_ctl));
	// mask
	asm volatile("msr CNTV_CTL_EL0, %0" :: "r" (armcp14->cntv_ctl | 2));

	if ((armcp14->cntv_ctl & 3) == 1) {
		uint64_t vcnt;
		asm volatile("mrs %0, CNTVCT_EL0" : "=r" (vcnt));
		armcp14->queue_item_virt.item.expiration = armcp14->cntv_cval + armcp14->vcval_read_at - vcnt;
		timequeue_insert(&armcp14->queue_item_virt);
	}
}

void emulate_timer_armcp14_load_state(const vm_cpu *vm_cpu_em, emulate_timer_armcp14 *armcp14) {
	//uint64_t phys_now;

	(void)vm_cpu_em;

	//phys_now = clock_read();

	/* adjust CNTVOFF (?) */
	// armcp14->cntvoff += (phys_now - armcp14->vcval_read_at);
	// asm volatile("msr CNTVOFF_EL2, %0" :: "r" (armcp14->cntvoff));

	/* reinstate timer (?) */
	if ( ((armcp14->cntv_ctl & 3) == 1) &&
		(armcp14->queue_item_virt.item.armed == 0) ) {
		asm volatile(	"mov x0, #3\n\t"
				"msr CNTV_CTL_EL0, x0\n\t"
				"1: mrs x0, CNTVCT_EL0\n\t"
				"msr CNTV_CVAL_EL0, x0\n\t"
				"isb\n\t"
				"mrs x0, CNTV_CTL_EL0\n\t"
				"tbz x0, #2, 1b" ::: "x0");
		// timer has expired while VM was descheduled
	} else {
		timequeue_delete(&armcp14->queue_item_virt);
		asm volatile("msr CNTV_CVAL_EL0, %0" :: "r" (armcp14->cntv_cval));
		asm volatile("msr CNTV_CTL_EL0, %0" :: "r" (armcp14->cntv_ctl));
		// timer is still armed... delete from HYP queue and reprogram
	}
}

void emulate_timer_armcp14_expired(timer *timer_expired) {
	const vm_cpu *vm_cpu_timer = timer_expired->queueing_guest;
	deliver_irq_to_vm(vm_cpu_timer->vm, 0x1b, 0);
	if (vm_cpu_timer == current_vm_cpu) {
		printf("Huh? CP14 EM timer expired on running VM...\r\n");
	}
}
