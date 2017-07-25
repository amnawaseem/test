#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <schism/xml42.h>
#include <schism/range.h>
#include "layouter.h"

// ---------------------------------------------------------------------------

uint32_t *mmu_gather_alignments(struct scenario_context *sctx, xmlNodePtr format) {
	uint32_t count, val, i;
	uint32_t *list;
	xmlNodePtr pformat, level;

	count = 0;
	iterate_over_children(sctx->doc, "/scenario/platform/arch", "paging_format", pformat) //{
		if (format && (pformat != format))
			continue;

		iterate_over_children_N(pformat, "level", level) //{
			++count;
		}
	}

	list = malloc((count+1) * sizeof(uint32_t));
	memset(list, 0, (count+1) * sizeof(uint32_t));
	count = 0;

	iterate_over_children(sctx->doc, "/scenario/platform/arch", "paging_format", pformat) //{
		if (format && (pformat != format))
			continue;

		iterate_over_children_N(pformat, "level", level) //{
			val = strtoul(get_attribute(level, "shift"), NULL, 0);
			for (i = 0; i < count; i++) {
				if (list[i] == val) {
					val = 0;
					break;
				} else if (list[i] < val) {
					uint32_t tmp = list[i];
					list[i] = val;
					val = tmp;
				}
			}
			list[count] = val;
			++count;
		}
	}

	return list;
}
