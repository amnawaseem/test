#ifndef	_PHIDIAS_SCHEDULE_H_
#define	_PHIDIAS_SCHEDULE_H_

#include <asm/schedule.h>

struct vm_cpu_;
struct timer_;

struct scheduler_entity_ {
	const struct vm_cpu_	*cpu;

	uint32_t	class;
	uint32_t	realtime_budget;
	uint32_t	realtime_period;
	uint32_t	fair_share;

	uint32_t	state;
	struct scheduler_entity_	*next;
};
typedef struct scheduler_entity_	scheduler_entity;

extern const struct vm_cpu_	*current_vm_cpu;
extern scheduler_entity		*current_sched_entity;

extern scheduler_entity		*sched_queue_head;
extern scheduler_entity		*sched_queue_tail;

extern void sched_start(void);
extern void sched_queue_add(struct scheduler_entity_ *);
extern void sched_queue_del(struct scheduler_entity_ *);
extern struct scheduler_entity_ *sched_queue_get(void);
extern void sched_state_modify(scheduler_entity *sched_ent, uint32_t new_state);

extern void scheduler_expired(struct timer_ *);
extern void reschedule(void);
extern void __attribute__((noreturn)) upcall(void);

#endif
