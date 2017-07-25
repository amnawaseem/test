#include <phidias.h>
#include <timequeue.h>
#include <vm.h>
#include <schedule.h>
#include <interrupts.h>
#include <drivers/timer.h>
#include <drivers/timer-arm-generic.h>
#include <drivers/clock.h>

void timer_arm_generic_setup() {
	// timer_arm_generic_el2_set_comparator(0x800000);
	// timer_arm_generic_el2_set_control(1);
}

void timer_arm_generic_program(timer *event) {
	timer_arm_generic_el2_set_comparator(event->expiration);
	timer_arm_generic_el2_set_control(1);
}

uint32_t timer_arm_generic_interrupt() {
	timer_arm_generic_el2_set_control(0);
	timequeue_interrupt();

	return 0;
}
