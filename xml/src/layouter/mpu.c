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

static int layout_memory_mpu_group(struct scenario_context *sctx, xmlNodePtr memory, uint32_t cpu, const char *flags, void *RM) {
  const char *base_str;
  xmlNodePtr memreq, hyp_as, map;
  uint64_t group_base = 0, group_size = 0;

  hyp_as = query_element(sctx->doc, "/scenario/hypervisor/address_space");

  iterate_over_children_N(memory, "memreq", memreq) //{
    base_str = get_attribute(memreq, "base");
    if (base_str != NULL)
      continue;

    map = find_child_by_attribute(hyp_as, "map", "xref", get_attribute(memreq, "id"));
    if (strcmp(get_attribute(map, "flags"), flags) != 0)
      continue;

    if ((get_attribute(memreq, "cpumap") != NULL) &&
		(!list_contains(memreq, "cpumap", cpu)))
      continue;

    group_size += strtoul(get_attribute(memreq, "size"), NULL, 0);
  }

  if (group_size == 0)
    return 0;

  // other archs might not need the alignment...
  group_size = LOG2_CEIL(group_size);

  if (find_range_one_aligned(RM, 0, (1UL << group_size), group_size, &group_base) != 0) {
    fprintf(stderr, "ERROR: cannot find space for group \"%s\" (cpu %d).\n", flags, cpu);
    return 1;
  }
  fprintf(stderr, "Group \"%s\" (cpu %d) placed at %lx+%lx.\n", flags, cpu, group_base, group_size);

  if (claim_range_one(RM, 0, group_base, group_size) != 0) {
    fprintf(stderr, "ERROR: cannot claim space for group \"%s\" (cpu %d).\n", flags, cpu);
    return 1;
  }

  group_size = 0;

  iterate_over_children_N(memory, "memreq", memreq) //{
    base_str = get_attribute(memreq, "base");
    if (base_str != NULL)
      continue;

    map = find_child_by_attribute(hyp_as, "map", "xref", get_attribute(memreq, "id"));
    if (strcmp(get_attribute(map, "flags"), flags) != 0)
      continue;

    if (get_attribute(memreq, "cpumap") != NULL) {
      if (!list_contains(memreq, "cpumap", cpu))
        continue;

      if (get_attribute(memreq, "base") == NULL)
        set_attribute(memreq, "base", "");
      dict_append_hexnumber(memreq, "base", cpu, group_base + group_size);
    } else {
      set_attribute_hexnumber(memreq, "base", group_base + group_size);
    }

    group_size += strtoul(get_attribute(memreq, "size"), NULL, 0);
  }

  return 0;
}

int layout_memory_mpu(struct scenario_context *sctx, xmlNodePtr memory) {
  uint64_t i;
  void *RM;
  uint32_t ncpus;

  RM = init_range_manager(strtoul(get_attribute(memory, "base"), NULL, 0),
			strtoul(get_attribute(memory, "size"), NULL, 0),
			1);
  if (RM == NULL) {
    fprintf(stderr, "ERROR: cannot initialize memory range manager.\n");
    return 1;
  }

  if (common_phys_fixed_to_rm(RM, memory)) {
    return 1;
  }

  if (layout_memory_mpu_group(sctx, memory, 0, "rwgs", RM))
    return 1;

  ncpus = strtoul(query_attribute(sctx->doc, "/scenario/hypervisor", "ncpus"), NULL, 0);
  for (i = 0; i < ncpus; i++) {
    if (layout_memory_mpu_group(sctx, memory, i, "rwg", RM))
      return 1;
  }

  if (layout_memory_mpu_group(sctx, memory, 0, NULL, RM))
    return 1;

  return 0;
}

// ---------------------------------------------------------------------------

int layout_map_mpu(struct scenario_context *sctx, xmlNodePtr address_space) {
  xmlNodePtr map;

  iterate_over_children_N(address_space, "map", map) //{
    xmlNodePtr xref = resolve(sctx->doc, get_attribute(map, "xref"));
    set_attribute(map, "base", get_attribute(xref, "base"));
  }

  return 0;
}
