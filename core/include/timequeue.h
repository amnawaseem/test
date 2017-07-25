#ifndef	_PHIDIAS_TIMEQUEUE_H_
#define	_PHIDIAS_TIMEQUEUE_H_

struct vm_cpu_;
struct emulate_;

struct timer_ {
	uint32_t	queueing_driver;
	uint32_t	queueing_subunit;
	const struct vm_cpu_	*queueing_guest;
	void		*queueing_device;
	uint64_t	expiration;
	uint32_t	armed;
};
typedef struct timer_	timer;

struct queue_timer_ {
	struct queue_timer_	*next;
	struct timer_		item;
};
typedef struct queue_timer_	queue_timer;

#define	TIMERDRIVER_SCHEDULER		1
#define	TIMERDRIVER_EMULATE_MPCORE	2
#define	TIMERDRIVER_EMULATE_SP804	3
#define	TIMERDRIVER_EMULATE_ARMCP14	4
#define	TIMERDRIVER_EMULATE_PIC32	5
#define	TIMERDRIVER_EMULATE_LAPIC	6

typedef void (timer_expiration_handler)(timer *);

extern queue_timer *timequeue;

extern void timequeue_delete(queue_timer *);
extern void timequeue_insert(queue_timer *);
extern void timequeue_interrupt(void);

#endif
