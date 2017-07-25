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

int layout_memory_flat(struct scenario_context *sctx, xmlNodePtr memory) {
  return 1;
#if 0
  uint64_t i;
  struct xmlnode *memreq;
  struct range *R, *R2;

  R = init_range(memory->attrs[MEMORY_ATTR_BASE].value.number,
		 memory->attrs[MEMORY_ATTR_SIZE].value.number);
  if (!R) {
    fprintf(stderr, "ERROR: cannot initialize memory layouter.\n");
    return 1;
  }

  // step 1: claim all fixed ranges
  iterate_over_children(i, memory, STRUCT_MEMREQ, memreq) {
    if (attr_exists(memreq->attrs + MEMREQ_ATTR_BASE)) {
      if (memreq->attrs[MEMREQ_ATTR_BASE].value.dict.keys != ATTR_WILDCARD) {
        fprintf(stderr, "ERROR: cannot claim pre-placed percore <memreq> nodes yet.\n");
        return 1;
      }
      if (attr_exists(memreq->attrs + MEMREQ_ATTR_ALIGN_SHIFT) &&
	(memreq->attrs[MEMREQ_ATTR_BASE].value.number & ((1UL << memreq->attrs[MEMREQ_ATTR_ALIGN_SHIFT].value.number) - 1))) {
        fprintf(stderr, "ERROR: fixed <memreq> does not fulfil own \"align_shift\".\n");
        return 1;
      }
      R2 = claim_range(R,
		get_dict_hex(memreq->attrs + MEMREQ_ATTR_BASE, 0),
		memreq->attrs[MEMORY_ATTR_SIZE].value.number);
      if (!R2) {
        fprintf(stderr, "ERROR: cannot claim \"%s\".\n", memreq->attrs[MEMREQ_ATTR_ID].value.string);
        error_dump_ranges(R);
        return 1;
      }
      R = R2;
    }
  }

  // step 2: layout all the rest freely as we see fit
  iterate_over_children(i, memory, STRUCT_MEMREQ, memreq) {
    uint64_t base, size;
    uint32_t align_req;

    if (attr_exists(memreq->attrs + MEMREQ_ATTR_BASE))
      continue;

    if (attr_exists(memreq->attrs + MEMREQ_ATTR_ALIGN_SHIFT))
      align_req = memreq->attrs[MEMREQ_ATTR_ALIGN_SHIFT].value.number;
    else
      align_req = 2;

    size = memreq->attrs[MEMORY_ATTR_SIZE].value.number;

    if (attr_exists(memreq->attrs + MEMREQ_ATTR_CPUMAP)) {
      for (i = 0; i < get_child(scene, STRUCT_HYPERVISOR)->attrs[HYPERVISOR_ATTR_NCPUS].value.number; i++) {
        if (has_list(memreq->attrs + MEMREQ_ATTR_CPUMAP, i)) {
          if (find_range(R, size, align_req, &base) == NULL) {
            fprintf(stderr, "ERROR: cannot place percpu <memreq> \"%s\".\n", memreq->attrs[MEMREQ_ATTR_ID].value.string);
            return 1;
          }
          add_dict_hex(memreq->attrs + MEMREQ_ATTR_BASE, i, base);
          R2 = claim_range(R, base, size);
          if (R2 == NULL) {
            fprintf(stderr, "ERROR: cannot claim percpu <memreq> \"%s\".\n", memreq->attrs[MEMREQ_ATTR_ID].value.string);
            return 1;
          }
          R = R2;
        }
      }
    } else {
      if (find_range(R, size, align_req, &base) == NULL) {
        fprintf(stderr, "ERROR: cannot place <memreq> \"%s\".\n", memreq->attrs[MEMREQ_ATTR_ID].value.string);
        return 1;
      }
      memreq->attrs[MEMREQ_ATTR_BASE].attr_type = ATTRTYPE_HEX;
      memreq->attrs[MEMREQ_ATTR_BASE].state = ATTRSTATE_MODIFIED;
      memreq->attrs[MEMREQ_ATTR_BASE].value.number = base;
      R2 = claim_range(R, base, size);
      if (R2 == NULL) {
        fprintf(stderr, "ERROR: cannot claim <memreq> \"%s\".\n", memreq->attrs[MEMREQ_ATTR_ID].value.string);
        return 1;
      }
      R = R2;
    }
  }

  return 0;
#endif
}
