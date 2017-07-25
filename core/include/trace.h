#ifndef	_PHIDIAS_TRACE_H_
#define	_PHIDIAS_TRACE_H_

#if defined(FEATURE_TRACE)

# include <drivers/clock.h>

struct trace_line_ {
	uint64_t	time;
	uint32_t	value[4];
	char		label[8];
};
typedef struct trace_line_	trace_line;

extern trace_line *trace_cursor;
extern uintptr_t tracebuffer_start;
extern uintptr_t tracebuffer_end;

static inline void trace(const char *label, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
	uint32_t i = 0;

	trace_cursor->time = clock_read();

	while (i < 8) {
		trace_cursor->label[i] = label[i];
		if (!label[i])
			break;
		++i;
	}

	trace_cursor->value[0] = a;
	trace_cursor->value[1] = b;
	trace_cursor->value[2] = c;
	trace_cursor->value[3] = d;

	trace_cursor++;
	if (trace_cursor == (trace_line *)tracebuffer_end) {
		trace_cursor = (trace_line *)tracebuffer_start;
	}
}

#else /* FEATURE_TRACE */

# define trace(label,a,b,c,d)
# define trace_setup()
# define trace_dump()

#endif

#endif
