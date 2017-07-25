#ifndef	_PHIDIAS_EMULATE_CLOCK_H_
#define	_PHIDIAS_EMULATE_CLOCK_H_

struct emulate_clock_mpcore_ {
	uint64_t	value;
	uint64_t	host_reference;
	/* multiplier/divider against host clock? */
};
typedef struct emulate_clock_mpcore_	emulate_clock_mpcore;

#endif
