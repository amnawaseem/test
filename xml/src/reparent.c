#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>

xmlNodePtr default_memory = NULL;

// ---------------------------------------------------------------------------

static const char *gen_sequence(uint64_t n) {
  char *s = malloc(64);
  int w = sprintf(s, "%lu", n);
  uint64_t i;

  s = realloc(s, w * n + 4);
  w = sprintf(s, "[");

  for (i = 0; i < n; i++) {
    w += sprintf(s+w, "%lu,", i);
  }

  sprintf(s+w-1, "]");

  return s;
}

static const char *order_mapflags(const char *flags_in) {
  uint32_t i;
  const char *order = "rwxgdus";
  char *flags_out = malloc(strlen(flags_in) + 1);
  char *p = flags_out;

  for (i = 0; i < strlen(order); i++) {
    if (strchr(flags_in, order[i])) {
      *(p++) = order[i];
    }
  }

  *p = 0;
  return flags_out;
}

static int check_memreqs(struct scenario_context *sctx, xmlNodePtr memreq, const char *cpumap) {
  const char *memreq_id, *memreq_on;
  xmlNodePtr matched_map = NULL;
  xmlNodePtr as;

  memreq_id = get_attribute(memreq, "id");

  // set default "on" attribute for <memreq> nodes
  memreq_on = get_attribute(memreq, "on");
  if (memreq_on == NULL) {
    set_attribute(memreq, "on", get_attribute(default_memory, "id"));
    fprintf(stderr, "INFO: Added \"on\" attribute to <memreq> node \"%s\".\n", memreq_id);
  }

  as = query_element(sctx->doc, "/scenario/hypervisor/address_space");
  matched_map = find_child_by_attribute(as, "map", "xref", memreq_id);

  if (matched_map == NULL) {
    const char *flags_demand = get_attribute(memreq, "flags_demand");
    if (flags_demand && (strlen(flags_demand) > 0)) {
      // <memreq> not contained in <address_space>, but requires it --> add node
      matched_map = xmlNewDocNode(sctx->doc, NULL, (const xmlChar *)"map", NULL);
      set_attribute(matched_map, "xref", memreq_id);
      set_attribute(matched_map, "flags", flags_demand);
      xmlAddChild(as, matched_map);
      fprintf(stderr, "INFO: Added <map> for required <memreq> \"%s\".\n", memreq_id);
    }
  }

  // if <map> is constrained by a guest cpumap, constrain its mapping for the hypervisor accordingly,
  // even if the mapping is flagged global; the 'g' bit specifies "all guest VCPUs" and furthermore
  // requires placement in the hypervisor global window, it doesn't specify "all host CPUs"
  if (matched_map && cpumap) {
    set_attribute(matched_map, "cpumap", cpumap);
    fprintf(stderr, "INFO: Constrained <map> for guest <memreq> \"%s\" to the guest's cpumap.\n",
		memreq_id);
  }

  return 0;
}

static int check_map_attributes(struct scenario_context *sctx) {
  unsigned int i, j;
  xmlNodePtr map;
  xmlNodePtr memdev;		// memreq or device

  iterate_over_children(sctx->doc, "/scenario/hypervisor/address_space", "map", map) //{
    const char *map_xref, *map_flags;
    const char *flags_prevent, *flags_demand;
    char flags_new[16];

    map_xref = get_attribute(map, "xref");
    map_flags = get_attribute(map, "flags");

    memdev = resolve(sctx->doc, map_xref);

    if (memdev == NULL) {
      fprintf(stderr, "ERROR: neither <memreq> nor <device> found for <map> \"%s\".\n",
			map_xref);
      return 1;
    } else if (strcmp((const char *)memdev->name, "memreq") == 0) {
      flags_prevent = get_attribute(memdev, "flags_prevent");
      flags_demand = get_attribute(memdev, "flags_demand");
    } else if (strcmp((const char *)memdev->name, "device") == 0) {
      flags_prevent = "xus";
      flags_demand = "rgd";
    } else {
      fprintf(stderr, "ERROR: <map> xref \"%s\" resolves to strange XML node.\n", map_xref);
      return 1;
    }

    if (map_flags == NULL) {
      map_flags = "";
    }

    if (map_flags && (strcspn(map_flags, flags_prevent) != strlen(map_flags))) {
      fprintf(stderr, "ERROR: <map> \"%s\" contains forbidden flags.\n", map_xref);
      return 1;
    }

    snprintf(flags_new, 16, "%s", map_flags);
    i = strlen(flags_new);

    for (j = 0; j < strlen(flags_demand); j++) {
      if (!strchr(flags_new, flags_demand[j])) {
        if (i == 15) {
          fprintf(stderr, "ERROR: <map> \"%s\" flags overflow.\n", map_xref);
          return 1;
        }
        flags_new[i] = flags_demand[j];
        ++i;
        flags_new[i] = 0;
      }
    }
    if (strcmp(flags_new, map_flags) != 0) {
      fprintf(stderr, "INFO: Added required flag to <map> \"%s\" (\"%s\" => \"%s\").\n",
                        map_xref, map_flags, flags_new);
    }

    set_attribute(map, "flags", order_mapflags(flags_new));
  }

  return 0;
}

// ---------------------------------------------------------------------------

int main(int an, char **ac) {
  struct scenario_context sctx;
  xmlNodePtr memory, guest;
  uint64_t default_memory_size;
  xmlNodePtr memreq, next_memreq;
  const char *percore_all;

  memset(&sctx, 0, sizeof(sctx));

  if (an != 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = SCENARIO_STAGE_EXPANDED;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  // ---------------------------------------------------------------------------

  // determine default <memory> node
  default_memory = NULL;
  default_memory_size = 0;

  iterate_over_children(sctx.doc, "/scenario/platform/board", "memory", memory) //{
    uint64_t this_memory_size = strtoul(get_attribute(memory, "size"), NULL, 0);
    if (this_memory_size > default_memory_size) {
      default_memory = memory;
      default_memory_size = this_memory_size;
    }
  }
  if (!default_memory) {
    fprintf(stderr, "ERROR: no <memory> node found.\n");
    return 1;
  }

  // ---------------------------------------------------------------------------

  // check <memreq> flags, create implicit <map> nodes, determine <memory> node
  iterate_over_children(sctx.doc, "/scenario/hypervisor", "memreq", memreq) //{
    check_memreqs(&sctx, memreq, NULL);
  }

  iterate_over_children(sctx.doc, "/scenario", "guest", guest) //{
    const char *guest_cpumap = get_attribute(guest, "cpumap");
    iterate_over_children_N(guest, "memreq", memreq) //{
      check_memreqs(&sctx, memreq, guest_cpumap);
    }
  }

  // ---------------------------------------------------------------------------

  // reparent <memreq> nodes to associated <memory> node
  percore_all = gen_sequence(strtoul(query_attribute(sctx.doc, "/scenario/hypervisor", "ncpus"), NULL, 0));

  for (memreq = query_element(sctx.doc, "/scenario/hypervisor")->children;
       memreq;
       memreq = next_memreq) {
    const char *memreq_on, *memreq_cpumap;
    xmlNodePtr memory;

    if (strcmp((const char *)memreq->name, "memreq") != 0) {
      next_memreq = memreq->next;
      continue;
    }

    memreq_on = get_attribute(memreq, "on");
    memreq_cpumap = get_attribute(memreq, "cpumap");

    memory = resolve(sctx.doc, memreq_on);
    if (memory == NULL) {
      fprintf(stderr, "ERROR: unknown <memory> node \"%s\".\n", memreq_on);
      return 1;
    }

    if (memreq_cpumap && (strcmp(memreq_cpumap, "*") == 0)) {
      set_attribute(memreq, "cpumap", percore_all);
      fprintf(stderr, "INFO: expanded percore attribute when reparenting memreq \"%s\".\n",
		get_attribute(memreq, "id"));
    }

    next_memreq = memreq->next;

    xmlAddChild(memory, xmlCopyNode(memreq, -1));
    xmlUnlinkNode(memreq);
  }

  iterate_over_children(sctx.doc, "/scenario", "guest", guest) //{
    const char *guest_cpumap = get_attribute(guest, "cpumap");
    for (memreq = guest->children; memreq; memreq = next_memreq) {
      const char *memreq_on, *memreq_cpumap;
      xmlNodePtr memory;

      if (strcmp((const char *)memreq->name, "memreq") != 0) {
        next_memreq = memreq->next;
        continue;
      }

      memreq_on = get_attribute(memreq, "on");
      memreq_cpumap = get_attribute(memreq, "cpumap");

      memory = resolve(sctx.doc, memreq_on);
      if (memory == NULL) {
        fprintf(stderr, "ERROR: unknown <memory> node \"%s\".\n", memreq_on);
        return 1;
      }

      if (memreq_cpumap && (strcmp(memreq_cpumap, "*") != 0)) {
        fprintf(stderr, "ERROR: cannot reparent <memreq> node \"%s\" - has custom cpumap attribute.\n",
			get_attribute(memreq, "id"));
        return 1;
      }

      set_attribute(memreq, "cpumap", guest_cpumap);
      fprintf(stderr, "INFO: reduced percore attribute when reparenting memreq \"%s\".\n",
			get_attribute(memreq, "id"));

      next_memreq = memreq->next;

      xmlAddChild(memory, xmlCopyNode(memreq, -1));
      xmlUnlinkNode(memreq);
    }
  }

  // ---------------------------------------------------------------------------

  // check <map> nodes in hypervisor again, enforce flags
  check_map_attributes(&sctx);

  // restore schema-mandated element order
  sort_children(&sctx, query_element(sctx.doc, "/scenario/hypervisor/address_space"));
  iterate_over_children(sctx.doc, "/scenario/platform/board", "memory", memory) //{
    sort_children(&sctx, memory);
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
