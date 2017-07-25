#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <schism/xml42.h>
#include "pagetables.h"

void init_map_iterator(struct map_iterator *iter, struct scenario_context *sctx, xmlNodePtr address_space, uint32_t cpu) {
  iter->sctx = sctx;
  iter->addr_space = address_space;
  iter->cpu = cpu;

  iter->map = NULL;
  iter->xref = NULL;
  iter->pa = 0;
  iter->size = 0;
}

int iterate_maps(struct map_iterator *iter) {
  const char *tmp;

  if (iter->map == NULL)
    iter->map = iter->addr_space->children;
  else
    iter->map = iter->map->next;

  if (iter->map == NULL)
    return 1;

  iter->xref = resolve(iter->sctx->doc, get_attribute(iter->map, "xref"));
  iter->size = get_attribute_number(iter->xref, "size");

  tmp = get_attribute(iter->xref, "base");
  if (tmp[0] == '[') {
    iter->pa = dictS_get_hexnumber(tmp, iter->cpu);
  } else {
    iter->pa = strtoul(tmp, NULL, 0);
  }

  if (get_attribute(iter->map, "offset") != NULL) {
    iter->pa += get_attribute_number(iter->map, "offset");
    // DO NOT INCREMENT va
    iter->size -= get_attribute_number(iter->map, "offset");
  }
  if (get_attribute(iter->map, "subsize") != NULL) {
    iter->size = get_attribute_number(iter->map, "subsize");
  }

  return 0;
}
