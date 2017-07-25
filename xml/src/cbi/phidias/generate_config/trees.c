#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

// --------------------------------------------------------------------------

void generate_memtree(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr address_space,
			uint32_t memarea_count, uint32_t cpu,
			const char *memarea_name,
			const char *top_item_name, const char *top_item_xref) {
  xmlNodePtr *sorted_maps, map;
  uint64_t *sorted_vas, va;
  uint32_t j;
  uint32_t sort_interval[2];

  sorted_maps = malloc(memarea_count * sizeof(void *));
  memset(sorted_maps, 0, memarea_count * sizeof(void *));
  sorted_vas = malloc(memarea_count * sizeof(uint64_t));

  // iterate over <map>, sort by VA
  iterate_over_children_N(address_space, "map", map) //{
    if ((get_attribute(map, "cpumap") != NULL) &&
		(!list_contains(map, "cpumap", cpu)))
      continue;

    get_map_properties(sctx, map, cpu, cpu, NULL, &va, NULL);

    for (j = 0; j < memarea_count; j++) {
      if (sorted_maps[j] == NULL) break;
      if (va < sorted_vas[j]) break;
    }
    memmove(sorted_maps + j + 1, sorted_maps + j, (memarea_count - j - 1) * sizeof(void *));
    sorted_maps[j] = map;
    memmove(sorted_vas + j + 1, sorted_vas + j, (memarea_count - j - 1) * sizeof(uint32_t));
    sorted_vas[j] = va;
  }

  // we may have had holes in our list, so update memarea_count
  for (j = 0; j < memarea_count; j++) {
    if (sorted_maps[j] == NULL) break;
  }
  memarea_count = j;

  sort_interval[0] = 0;
  sort_interval[1] = memarea_count;

  // split first time at top_item_xref, else mid
  if (top_item_xref) {
    for (j = 0; j < memarea_count; j++) {
      if (strcmp(get_attribute(sorted_maps[j], "xref"), top_item_xref) == 0)
        break;
    }
  } else {
    j = memarea_count/2;
  }

  generate_memtree_item(A, sorted_maps, sort_interval, j, memarea_name, top_item_name);

  free(sorted_maps);
  free(sorted_vas);
}

void generate_memtree_item(struct addendum *A,
			xmlNodePtr *sortlist, uint32_t *sort_interval,
			uint32_t split_item,
			const char *memarea_name, const char *name) {
  struct definition *def;
  char *left_child, *right_child;
  uint32_t recurse_sort_interval[2];

  if (split_item - sort_interval[0] >= 1) {
    left_child = malloc(strlen(name) + 4);
    sprintf(left_child, "&%s_l", name);
  } else
    left_child = "NULL";

  if (sort_interval[1] - split_item > 1) {
    right_child = malloc(strlen(name) + 4);
    sprintf(right_child, "&%s_r", name);
  } else
    right_child = "NULL";

  if (strcmp(left_child, "NULL")) {
    recurse_sort_interval[0] = sort_interval[0];
    recurse_sort_interval[1] = split_item;
    generate_memtree_item(A, sortlist, recurse_sort_interval,
			recurse_sort_interval[0] + (recurse_sort_interval[1] - recurse_sort_interval[0]) / 2,
			memarea_name, left_child+1);
  }

  def = new_addendum_definition(A, "tree_memarea", ADDSEC_RO);
  def_buffer_printf(&def->identifier, "%s", name);
  def_buffer_printf(&def->initializer, "{ %s, %s, %s + %ld }",
			left_child,
			right_child,
			memarea_name,
			get_attribute_number(sortlist[split_item], "index"));

  if (strcmp(right_child, "NULL")) {
    recurse_sort_interval[0] = split_item+1;
    recurse_sort_interval[1] = sort_interval[1];
    generate_memtree_item(A, sortlist, recurse_sort_interval,
			recurse_sort_interval[0] + (recurse_sort_interval[1] - recurse_sort_interval[0]) / 2,
			memarea_name, right_child+1);
  }
}

// --------------------------------------------------------------------------

void generate_emulatetree(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, uint32_t emulate_count, uint32_t cpu,
			const char *emulate_name, const char *top_item_name) {
  xmlNodePtr *sorted_emulates;
  uint32_t *indices;
  xmlNodePtr vdev, emulate;
  uint32_t count = 0;
  uint32_t i, j;
  uint32_t sort_interval[2];

  (void)sctx; (void)cpu;

  sorted_emulates = malloc(emulate_count * sizeof(void *));
  memset(sorted_emulates, 0, emulate_count * sizeof(void *));
  indices = malloc(emulate_count * sizeof(uint32_t));
  memset(indices, 0, emulate_count * sizeof(uint32_t));

  // iterate over <emulate>, sort by VA
  iterate_over_children_N(guest, "vdev", vdev) //{
    i = 0;
    iterate_over_children_N(vdev, "emulate", emulate) //{
      for (j = 0; j < emulate_count; j++) {
        if (sorted_emulates[j] == NULL) break;
        if (get_attribute_number(emulate, "base") <
		get_attribute_number(sorted_emulates[j], "base"))
          break;
      }
      memmove(sorted_emulates + j + 1, sorted_emulates + j, (emulate_count - j - 1) * sizeof(void *));
      memmove(indices + j + 1, indices + j, (emulate_count - j - 1) * sizeof(uint32_t));
      sorted_emulates[j] = emulate;
      indices[j] = count + i;
      ++i;
    }
    count += (count_children(vdev, "emulate") ?: 1);
  }

  // there may have been <vdev> nodes without <emulate>, so update emulate_count
  for (j = 0; j < emulate_count; j++) {
    if (sorted_emulates[j] == NULL) break;
  }
  emulate_count = j;

  sort_interval[0] = 0;
  sort_interval[1] = emulate_count;

  // start splitting in half and recursing
  j = emulate_count/2;

  generate_emulatetree_item(A, sorted_emulates, indices, sort_interval, j, emulate_name, top_item_name);

  free(sorted_emulates);
  free(indices);
}

void generate_emulatetree_item(struct addendum *A,
			xmlNodePtr *sortlist, uint32_t *indices,
			uint32_t *sort_interval, uint32_t split_item,
			const char *emulate_name, const char *name) {
  struct definition *def;
  char *left_child, *right_child;
  uint32_t recurse_sort_interval[2];

  if (split_item - sort_interval[0] >= 1) {
    left_child = malloc(strlen(name) + 4);
    sprintf(left_child, "&%s_l", name);
  } else
    left_child = "NULL";

  if (sort_interval[1] - split_item > 1) {
    right_child = malloc(strlen(name) + 4);
    sprintf(right_child, "&%s_r", name);
  } else
    right_child = "NULL";

  if (strcmp(left_child, "NULL")) {
    recurse_sort_interval[0] = sort_interval[0];
    recurse_sort_interval[1] = split_item;
    generate_emulatetree_item(A, sortlist, indices, recurse_sort_interval,
			recurse_sort_interval[0] + (recurse_sort_interval[1] - recurse_sort_interval[0]) / 2,
			emulate_name, left_child+1);
  }

  def = new_addendum_definition(A, "tree_emulate", ADDSEC_RO);
  def_buffer_printf(&def->identifier, "%s", name);
  def_buffer_printf(&def->initializer, "{ %s, %s, %s + %d }",
			left_child,
			right_child,
			emulate_name,
			indices[split_item]);

  if (strcmp(right_child, "NULL")) {
    recurse_sort_interval[0] = split_item+1;
    recurse_sort_interval[1] = sort_interval[1];
    generate_emulatetree_item(A, sortlist, indices, recurse_sort_interval,
			recurse_sort_interval[0] + (recurse_sort_interval[1] - recurse_sort_interval[0]) / 2,
			emulate_name, right_child+1);
  }
}
