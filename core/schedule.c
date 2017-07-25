#include <phidias.h>
#include <schedule.h>
#include <vm.h>
#include <drivers/clock.h>
#include <timequeue.h>
#include <specification.h>
#include <misc_ops.h>
#include <emulate/core.h>
#include <emulate/irq.h>
#include <arch/cpu_ops.h>
#include <trace.h>
#include <arch/vm.h>
#include <arch/cpu_state.h>

const vm_cpu *current_vm_cpu = NULL;
scheduler_entity *current_sched_entity = NULL;

scheduler_entity *sched_queue_head = NULL;
scheduler_entity *sched_queue_tail = NULL;

static queue_timer schedule_timer;
static uint32_t reschedule_pending = 0;

void sched_start() {
	uint32_t vm_cpu_iterator;
	uint32_t copy_iter;
	const vm_cpu *vm_cpu_i;

	for (vm_cpu_iterator = 0;
		vm_cpu_iterator < _specification.cpus[cpu_number].num_vm_cpus;
		vm_cpu_iterator++) {
		vm_cpu_i = _specification.cpus[cpu_number].vm_cpus[vm_cpu_iterator];

		for (copy_iter = 0; copy_iter < vm_cpu_i->vm->num_copyins; copy_iter++) {
			memcpy((void *)vm_cpu_i->vm->copyins[copy_iter].dest,
				(void *)vm_cpu_i->vm->copyins[copy_iter].source,
				vm_cpu_i->vm->copyins[copy_iter].size);
		}
		cpu_dcache_flush(1, 0);
		//asm volatile("isb\n\tic iallu\n\tisb");

		vm_cpu_initialize(vm_cpu_i);
		emulate_initialize_all(vm_cpu_i);
		sched_queue_add(vm_cpu_i->sched_entities + 0);
	}

	scheduler_expired(&schedule_timer.item);
}

// --------------------------------------------------------------------------

void sched_queue_add(scheduler_entity *add_sched_ent) {
	if (add_sched_ent->state != SCHEDULER_STATE_READY) {
		panic();
	}

	if (sched_queue_tail == NULL) {
		sched_queue_head = add_sched_ent;
		sched_queue_tail = add_sched_ent;
	} else {
		sched_queue_tail->next = add_sched_ent;
	}
	add_sched_ent->next = NULL;
}

void sched_queue_del(scheduler_entity *del_sched_ent) {
	scheduler_entity **head_ptr = &sched_queue_head;

	while (*head_ptr) {
		if (*head_ptr == del_sched_ent) {
			*head_ptr = del_sched_ent->next;
			break;
		}
	}

	if (sched_queue_head == NULL) {
		sched_queue_tail = NULL;
	}
}

scheduler_entity *sched_queue_get() {
	scheduler_entity *head = sched_queue_head;

	if (head != NULL) {
		sched_queue_head = head->next;
	}
	if (sched_queue_head == NULL) {
		sched_queue_tail = NULL;
	}

	return head;
}

void sched_state_modify(scheduler_entity *sched_ent, uint32_t new_state) {
	uint32_t old_state = sched_ent->state;

	if (old_state == new_state) {
		return;
	}

	sched_ent->state = new_state;

	switch (new_state) {
	case SCHEDULER_STATE_READY:
		if (sched_ent != current_sched_entity) {
			sched_queue_add(sched_ent);
			return;
		}
		break;
	case SCHEDULER_STATE_WAITING:
	case SCHEDULER_STATE_DORMANT:
	case SCHEDULER_STATE_ZOMBIE:
		if (sched_ent != current_sched_entity) {
			sched_queue_del(sched_ent);
			return;
		} else {
			reschedule();
		}
	}
}

// --------------------------------------------------------------------------

static queue_timer schedule_timer = {
	NULL, { TIMERDRIVER_SCHEDULER, 0, NULL, NULL, 0L, 0 }
};

void scheduler_expired(timer *t) {
	uint64_t clock = clock_read();
	t->expiration = clock + 0x10000000ULL;
	// printf("tick. new item: %016qx, clk_read: %016qx\r\n", t->expiration, clock);
	timequeue_insert(&schedule_timer);
	reschedule_pending = 1;
}

void reschedule() {
	scheduler_entity *next_running;

	next_running = sched_queue_get();
	trace("sched", (uintptr_t)current_vm_cpu, next_running ? (uintptr_t)next_running->cpu : 0, 0, 0);
	// printf("Resched %x\r\n", next_running);

	if (next_running != NULL) {
		if (current_vm_cpu != NULL) {
			// do post-accounting: budget replenish etc.
			vm_cpu_save_emulates(current_vm_cpu);
			vm_cpu_state_save_full(current_vm_cpu);
			if (current_sched_entity->state == SCHEDULER_STATE_READY) {
				sched_queue_add(current_sched_entity);
			}
		}
		current_sched_entity = next_running;
		current_vm_cpu = next_running->cpu;
		vm_cpu_state_load_full(current_vm_cpu);
		vm_cpu_load_emulates(current_vm_cpu);
	} else {
		if (current_vm_cpu == NULL) {
			cpu_idle();
		} else if (current_sched_entity->state != SCHEDULER_STATE_READY) {
			vm_cpu_save_emulates(current_vm_cpu);
			vm_cpu_state_save_full(current_vm_cpu);
			current_sched_entity = NULL;
			current_vm_cpu = NULL;
			cpu_idle();
		}
	}

	// do pre-accounting: calculate available time slice, set timer
	upcall();
}

// --------------------------------------------------------------------------

void __attribute__((noreturn)) upcall() {
	uint32_t pending_interrupt;

	if (current_vm_cpu == NULL) {
		reschedule();
	}
	if (current_sched_entity->state != SCHEDULER_STATE_READY) {
		reschedule();
	}
	if (reschedule_pending) {
		reschedule_pending = 0;
		reschedule();
	}

	pending_interrupt = emulate_irq_activate_highest_interrupt(current_vm_cpu);
	if (pending_interrupt != NO_PENDING_INTERRUPT) {
		vm_cpu_inject_interrupt(current_vm_cpu, pending_interrupt);
	}

	vm_cpu_upcall();
}
