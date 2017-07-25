#include <schism/xml42.h>
#include <schism/gc.h>
#include <schism/util.h>
#include "generate_config.h"

#define	HYPERVISOR_FIXED_MEMAREA_COUNT		32

struct memarea_fixed_index {
  const char *id;
  uint32_t index;
};

static const struct memarea_fixed_index hypervisor_fixed_memareas[] = {
  { "core_rx",		0, },
  { "core_r",		1, },
  { "core_rw",		2, },
  { "core_rws",		3, },
  { "core_rwt",		4, },
  { "pagetables",	5, },
  { "blob",		6, },
  { "stack",		7, },
  { "config_r",		8, },
  { "config_rw",	9, },
  { "config_rws",	10, },
  { "config_rwt",	11, },
  { "xcore",		12, },
  { "trace",		13, },

  { "serial",		16, },
  { "irqc",		17, },
  { "mpcore",		17, },

  { "cs7a",		24, },

  { "mbox",		24, },

  { "gpio03",		24, },
  { "gpio4j",		25, },
  { "timers",		26, },
  { "thermal",		27, },

  { "lapic",		24, },
  { "ioapic",		25, },
  { "vmcx",		28, },

  { "CATCHALL",		31, },
  
  { "irq_pic32",	24, },
  { "timer_pic32",	25, },
};

uint32_t hypervisor_memarea_index(xmlNodePtr map) {
  const char *map_xref = get_attribute(map, "xref");
  uint32_t array_index;

  for (array_index = 0; array_index < ARRAYLEN(hypervisor_fixed_memareas); array_index++) {
    if (strcmp(map_xref, hypervisor_fixed_memareas[array_index].id) == 0) {
      return hypervisor_fixed_memareas[array_index].index;
    }
  }

  return HYPERVISOR_FIXED_MEMAREA_COUNT;
}

char *map_flags_string(const char *flags_attribute) {
  char *map_flags = malloc(16 * strlen(flags_attribute));

  if ((flags_attribute == NULL) || (strlen(flags_attribute) == 0)) {
    return "0";
  }

  map_flags[0] = '\0';

  if (strchr(flags_attribute, 'r'))
    sprintf(map_flags + strlen(map_flags), "|MEMAREA_FLAG_R");
  if (strchr(flags_attribute, 'w'))
    sprintf(map_flags + strlen(map_flags), "|MEMAREA_FLAG_W");
  if (strchr(flags_attribute, 'x'))
    sprintf(map_flags + strlen(map_flags), "|MEMAREA_FLAG_X");
  if (strchr(flags_attribute, 'g'))
    sprintf(map_flags + strlen(map_flags), "|MEMAREA_FLAG_G");
  if (strchr(flags_attribute, 'd'))
    sprintf(map_flags + strlen(map_flags), "|MEMAREA_FLAG_D");
  if (strchr(flags_attribute, 'u'))
    sprintf(map_flags + strlen(map_flags), "|MEMAREA_FLAG_U");
  if (strchr(flags_attribute, 's'))
    sprintf(map_flags + strlen(map_flags), "|MEMAREA_FLAG_S");

  return map_flags + 1;
}

// --------------------------------------------------------------------------

uint32_t hypervisor_memarea_count(struct scenario_context *sctx, uint32_t physical_cpu) {
  xmlNodePtr map;
  uint32_t count = HYPERVISOR_FIXED_MEMAREA_COUNT;
  uint32_t memarea_index;

  iterate_over_children(sctx->doc, "/scenario/hypervisor/address_space", "map", map) //{
    if ((get_attribute(map, "cpumap") != NULL) &&
		(!list_contains(map, "cpumap", physical_cpu)))
      continue;

    memarea_index = hypervisor_memarea_index(map);

    if (memarea_index < HYPERVISOR_FIXED_MEMAREA_COUNT) {
      set_attribute_number(map, "index", memarea_index);
    } else {
      set_attribute_number(map, "index", count);
      ++count;
    }
  }

  return count;
}

// --------------------------------------------------------------------------

uint32_t guest_memarea_count(xmlNodePtr guest) {
  xmlNodePtr guest_as = get_child(guest, "address_space");
  xmlNodePtr map;
  uint64_t i;

  i = 0;
  iterate_over_children_N(guest_as, "map", map) //{
    set_attribute_number(map, "index", i);
    ++i;
  }

  return i;
}

// --------------------------------------------------------------------------

void generate_body_memareas(struct def_buffer *dbf,
			struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr as, uint32_t memarea_count,
			uint32_t cpu, xmlNodePtr reference_as) {
  xmlNodePtr map;
  const char *map_xref_str;
  xmlNodePtr reference_map = NULL;
  xmlNodePtr *sorted_maps;
  uint64_t i;

  (void)A;

  sorted_maps = malloc(memarea_count * sizeof(void *));
  memset(sorted_maps, 0, memarea_count * sizeof(void *));

  iterate_over_children_N(as, "map", map) //{
    if ((get_attribute(map, "cpumap") != NULL) &&
		(!list_contains(map, "cpumap", cpu)))
      continue;

    sorted_maps[(uint32_t)get_attribute_number(map, "index")] = map;
  }

  def_buffer_printf(dbf, "{\n");

  for (i = 0; i < memarea_count; i++) {
    uint64_t paddr, vaddr, size;

    if (sorted_maps[i] == NULL) {
      def_buffer_printf(dbf, "  { 0, 0, 0, 0, NULL },\n");
      continue;
    }

    map_xref_str = get_attribute(sorted_maps[i], "xref");
    paddr = vaddr = size = 0;

    /* TODO: physical CPU may be different */
    get_map_properties(sctx, sorted_maps[i], cpu, cpu, &paddr, &vaddr, &size);

    def_buffer_printf(dbf, "  { 0x%lx, 0x%lx, 0x%lx, %s,",
			paddr, vaddr, size,
			map_flags_string(get_attribute(sorted_maps[i], "flags")));

    if (reference_as) {
      reference_map = find_child_by_attribute(reference_as, "map", "xref", map_xref_str);
    }

    if (reference_map) {
      def_buffer_printf(dbf, " cpu%d_memareas + %ld", cpu, get_attribute_number(reference_map, "index"));
    } else {
      def_buffer_printf(dbf, " NULL");
    }

    def_buffer_printf(dbf, " },\n");
  }
  def_buffer_printf(dbf, "}");

  free(sorted_maps);
}
