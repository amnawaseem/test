#ifndef	_PHIDIAS_LOCK_H_
#define	_PHIDIAS_LOCK_H_

extern void spinlock_lock(uint32_t *);
extern void spinlock_unlock(uint32_t *);
extern uint32_t spinlock_trylock(uint32_t *);

#endif
