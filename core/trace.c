#include <phidias.h>
#include <trace.h>
#include <misc_ops.h>

uintptr_t tracebuffer_start = 0;
uintptr_t tracebuffer_end = 0;
trace_line *trace_cursor = NULL;

void trace_setup() {
	tracebuffer_start = core_memarea(MEMAREA_TRACE)->vaddr;
	tracebuffer_end = tracebuffer_start + core_memarea(MEMAREA_TRACE)->size;

	trace_cursor = (trace_line *)tracebuffer_start;

	if (core_memarea(MEMAREA_TRACE)->size > 0) {
		memset((void *)tracebuffer_start, 0, tracebuffer_end - tracebuffer_start);
	}
}

void trace_dump() {
	const trace_line *dump_cursor = trace_cursor;
	uint32_t i = 0;

	if (core_memarea(MEMAREA_TRACE)->size == 0)
		return;

	do {
		if (dump_cursor->label[0] != '\0') {
			printf("[%5d] \"%8s\" %16x {%8x %8x %8x %8x}\r\n",
				i, dump_cursor->label,
				dump_cursor->time,
				dump_cursor->value[0],
				dump_cursor->value[1],
				dump_cursor->value[2],
				dump_cursor->value[3]);
			i++;
		}

		dump_cursor++;

		if ((uintptr_t)dump_cursor >= tracebuffer_end) {
			dump_cursor = (const trace_line *)tracebuffer_start;
		}
	} while (dump_cursor != trace_cursor);
}
