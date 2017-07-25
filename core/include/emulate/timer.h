#ifndef	_PHIDIAS_EMULATE_TIMER_H_
#define	_PHIDIAS_EMULATE_TIMER_H_

# include <timequeue.h>

struct vm_cpu_;

struct emulate_timer_sp804_instance_ {
	uint32_t	load;		// load value for future runs
	uint32_t	value;		// load value of current run
	uint32_t	control;

	uint64_t	value_set_at;
	struct queue_timer_	queue_item;
};
typedef struct emulate_timer_sp804_instance_	emulate_timer_sp804_instance;

struct emulate_timer_sp804_ {
	struct emulate_timer_sp804_instance_	instances[2];
	uint32_t	interrupt_line;
	uint32_t	interrupt_asserted;
};
typedef struct emulate_timer_sp804_	emulate_timer_sp804;

void emulate_timer_sp804_initialize(const struct vm_cpu_ *vm_cpu_reset, emulate_timer_sp804 *sp804);

void emulate_timer_sp804_store(const struct vm_cpu_ *vm_cpu_em, emulate_timer_sp804 *sp804,
		uint32_t bar, uintptr_t offset, uint32_t value);
uint32_t emulate_timer_sp804_load(const struct vm_cpu_ *vm_cpu_em, emulate_timer_sp804 *sp804,
		uint32_t bar, uintptr_t offset);

void emulate_timer_sp804_expired(struct timer_ *);

// ==========================================================================

struct emulate_timer_mpcore_ {
	uint32_t	load;
	uint32_t	value;
	uint32_t	control;

	uint64_t	value_set_at;
	uint32_t	interrupt_line;
	uint32_t	interrupt_asserted;

	struct queue_timer_	queue_item;
};
typedef struct emulate_timer_mpcore_	emulate_timer_mpcore;

void emulate_timer_mpcore_initialize(const struct vm_cpu_ *vm_cpu_reset, emulate_timer_mpcore *mpcore);

void emulate_timer_mpcore_store(const struct vm_cpu_ *vm_cpu_em, emulate_timer_mpcore *mpcore,
		uint32_t bar, uintptr_t offset, uint32_t value);
uint32_t emulate_timer_mpcore_load(const struct vm_cpu_ *vm_cpu_em, emulate_timer_mpcore *mpcore,
		uint32_t bar, uintptr_t offset);

void emulate_timer_mpcore_expired(struct timer_ *);

// ==========================================================================

struct emulate_timer_armcp14_ {
	struct queue_timer_	queue_item_virt;
	struct queue_timer_	queue_item_phys;

	// offset applied to this VM's virtual counter
	uint64_t	cntvoff;

	// real CNTPCT when saving the timer state
	uint64_t	vcval_read_at;
	// CP14 virtual timer compare value
	uint64_t	cntv_cval;
	uint32_t	cntv_ctl;

	// emulated part: physical timer (and real CNTPCT time it has been set)
	uint64_t	value_set_at;
	// CP14 physical timer compare value
	uint64_t	cntp_cval;
	uint32_t	cntp_ctl;
};
typedef struct emulate_timer_armcp14_	emulate_timer_armcp14;

void emulate_timer_armcp14_initialize(const struct vm_cpu_ *vm_cpu_reset, emulate_timer_armcp14 *armcp14);

uint32_t emulate_timer_armcp14_interrupt(void);

void emulate_timer_armcp14_save_state(const struct vm_cpu_ *vm_cpu_em, emulate_timer_armcp14 *armcp14);
void emulate_timer_armcp14_load_state(const struct vm_cpu_ *vm_cpu_em, emulate_timer_armcp14 *armcp14);

void emulate_timer_armcp14_expired(struct timer_ *);

#endif
