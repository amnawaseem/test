#include <phidias.h>
#include <timequeue.h>
#include <schedule.h>
#include <drivers/clock.h>
#include <drivers/timer.h>
#include <emulate/timer.h>
#include <misc_ops.h>
#include <trace.h>

queue_timer *timequeue = NULL;
static int timequeue_busy = 0;

static void timequeue_expire_item(queue_timer *expired_event) {
	expired_event->item.armed = 0;
	// trace("te", (uintptr_t)expired_event, expired_event->item.queueing_driver, 0, 0);
	switch (expired_event->item.queueing_driver) {
	case TIMERDRIVER_SCHEDULER:
		scheduler_expired(&expired_event->item);
		return;
#ifdef EMULATE_TIMER_MPCORE
	case TIMERDRIVER_EMULATE_MPCORE:
		emulate_timer_mpcore_expired(&expired_event->item);
		return;
#endif
#ifdef EMULATE_TIMER_SP804
	case TIMERDRIVER_EMULATE_SP804:
		emulate_timer_sp804_expired(&expired_event->item);
		return;
#endif
#ifdef EMULATE_TIMER_ARMCP14
	case TIMERDRIVER_EMULATE_ARMCP14:
		emulate_timer_armcp14_expired(&expired_event->item);
		return;
#endif
#ifdef EMULATE_IRQ_LAPIC
	case TIMERDRIVER_EMULATE_LAPIC:
		emulate_irq_lapic_expired(&expired_event->item);
		return;
#endif
	default:
		printf("Ouch, event (%x) w/ unknown timer driver (%d)!\r\n",
				expired_event, expired_event->item.queueing_driver);
		return;
	}
}

// --------------------------------------------------------------------------

void timequeue_delete(queue_timer *del_event) {
	queue_timer *old_head = timequeue;
	queue_timer **qtptr;

	del_event->item.armed = 0;

	for (qtptr = &timequeue; *qtptr; qtptr = &((*qtptr)->next)) {
		if (*qtptr == del_event) {
			*qtptr = del_event->next;
			del_event->next = NULL;
			break;
		}
	}

	if (old_head != timequeue) {
		timer_program(&timequeue->item);
	}
}

void timequeue_insert(queue_timer *new_event) {
	queue_timer *old_head = timequeue;
	queue_timer *predecessor;

	// trace("ti", (uintptr_t)new_event, new_event->item.queueing_driver, new_event->item.expiration, (uintptr_t)old_head);
	if (new_event->item.queueing_driver == 0) {
		printf("INVALID, AIEEEE: %x\r\n", (void *)__builtin_return_address(0));
		panic();
	}

	new_event->item.armed = 1;
	if ((timequeue == NULL) || (timequeue->item.expiration >= new_event->item.expiration)) {
		new_event->next = timequeue;
		timequeue = new_event;
	} else {
		for (predecessor = timequeue; (predecessor->next != NULL) &&
			(predecessor->next->item.expiration < new_event->item.expiration);
			predecessor = predecessor->next) {}

		new_event->next = predecessor->next;
		predecessor->next = new_event;
	}

	if (!timequeue_busy && (timequeue != old_head)) {
		timer_program(&timequeue->item);
	}
}

void timequeue_interrupt() {
	// trace("tq0", (uintptr_t)timequeue, timequeue ? (uintptr_t)timequeue->next : 0, timequeue ? timequeue->item.expiration : 0, 0);
	timequeue_busy = 1;

	while (timequeue && (timequeue->item.expiration <= clock_read())) {
		queue_timer *expired;

		expired = timequeue;
		timequeue = expired->next;
		expired->next = NULL;

		timequeue_expire_item(expired);
	}

	timequeue_busy = 0;

	if (timequeue) {
		timer_program(&timequeue->item);
	}
	// trace("tq1", (uintptr_t)timequeue, timequeue ? (uintptr_t)timequeue->next : 0, timequeue ? timequeue->item.expiration : 0, 0);
}
