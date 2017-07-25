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

int main(int an, char **ac) {
  struct scenario_context sctx;
  xmlNodePtr memory;
  const char *as_type;
  uint32_t *aligns;

  memset(&sctx, 0, sizeof(sctx));

  if (an != 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = SCENARIO_STAGE_MEASURED;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  // ---------------------------------------------------------------------------

  as_type = query_attribute(sctx.doc, "/scenario/hypervisor/address_space", "type");

  if (strcmp(as_type, "mmu") == 0) {
    uint32_t cnt;

    // get union of all desired alignments for this architecture
    aligns = mmu_gather_alignments(&sctx, NULL);
    for (cnt = 0; aligns[cnt]; cnt++)
      ;

    fprintf(stderr, "INFO: physical layout for MMU, alignments: %s\n", list_to_string(aligns, cnt));

    iterate_over_children(sctx.doc, "/scenario/platform/board", "memory", memory) //{
      if (layout_memory_mmu(memory, aligns)) {
        return 1;
      }
    }
  } else if (strcmp(as_type, "mpu") == 0) {
    fprintf(stderr, "INFO: physical layout for MPU\n");

    iterate_over_children(sctx.doc, "/scenario/platform/board", "memory", memory) //{
      if (layout_memory_mpu(&sctx, memory)) {
        return 1;
      }
    }
  } else if (strcmp(as_type, "flat") == 0) {
    fprintf(stderr, "INFO: physical layout for FLAT\n");

    iterate_over_children(sctx.doc, "/scenario/platform/board", "memory", memory) //{
      if (layout_memory_flat(&sctx, memory)) {
        return 1;
      }
    }
  }

  // ---------------------------------------------------------------------------

  sctx.stage++;
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate transformed scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  save_scenario(&sctx);

  // ---------------------------------------------------------------------------

  // VIRTUAL LAYOUTER
  if (strcmp(as_type, "mmu") == 0) {
    if (layout_map_mmu(&sctx, query_element(sctx.doc, "/scenario/hypervisor/address_space"))) {
      return 1;
    }
  } else if ((strcmp(as_type, "mpu") == 0) || (strcmp(as_type, "flat") == 0)) {
    fprintf(stderr, "INFO: virtual layout for MPU/FLAT\n");

    if (layout_map_mpu(&sctx, query_element(sctx.doc, "/scenario/hypervisor/address_space"))) {
      return 1;
    }
  }

  // ---------------------------------------------------------------------------

  sctx.stage++;
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate transformed scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  save_scenario(&sctx);

  return 0;
}
