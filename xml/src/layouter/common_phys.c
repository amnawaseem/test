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

int common_phys_fixed_to_rm(void *RM, xmlNodePtr memory) {
  xmlNodePtr memreq;
  const char *base_str;
  uint64_t base, size;
  uint32_t align_shift;
  int ret;

  iterate_over_children_N(memory, "memreq", memreq) //{
    base_str = get_attribute(memreq, "base");
    if (base_str == NULL)
      continue;

    if (base_str[0] == '[') {
      fprintf(stderr, "ERROR: cannot claim pre-placed percore <memreq> nodes yet.\n");
      return 1;
    }

    base = strtoul(base_str, NULL, 0);
    align_shift = strtoul(get_attribute(memreq, "align_shift") ?: "", NULL, 0);

    if (align_shift && (base & ALIGN_MASK(align_shift))) {
      fprintf(stderr, "ERROR: fixed <memreq> \"%s\" does not fulfil alignment (%lx, %d).\n",
			get_attribute(memreq, "id"), base, align_shift);
      return 1;
    }

    size = strtoul(get_attribute(memreq, "size"), NULL, 0);
    ret = claim_range_one(RM, 0, base, size);
    if (ret) {
      fprintf(stderr, "ERROR: cannot claim \"%s\".\n", get_attribute(memreq, "id"));
      return 1;
    }
  }

  return 0;
}
