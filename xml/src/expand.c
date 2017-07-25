#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>

static int populate_from_hypervisor(struct scenario_context *sctx) {
  xmlNodePtr hypervisor, addr_space, memreq, map;

  sctx->aux_category = "default_hypervisor";
  sctx->aux_name = query_attribute(sctx->doc, "/scenario", "cbi");
  if (load_auxiliary(sctx) != 0) {
    fprintf(stderr, "ERROR: cannot load default hypervisor declarations: %s.\n", sctx->error_buffer);
    return 1;
  }

  hypervisor = query_element(sctx->doc, "/scenario/hypervisor");
  iterate_over_children(sctx->auxDoc, "/hypervisor", "memreq", memreq) //{
    const char *memreq_id = get_attribute(memreq, "id");
    xmlNodePtr scene_memreq = resolve(sctx->doc, memreq_id);

    if (scene_memreq == NULL) {
      xmlNodePtr copied_memreq = xmlCopyNode(memreq, -1);
      xmlAddChild(hypervisor, copied_memreq);

      fprintf(stderr, "INFO: copied over default <memreq> for \"%s\".\n", memreq_id);
    } else {
      const char *va, *vb;

      va = get_attribute(memreq, "flags_demand");
      vb = get_attribute(scene_memreq, "flags_demand");
      if (strcmp(va, vb) != 0) {
        fprintf(stderr, "WARN: <memreq> for \"%s\" has odd \"flags_demand\" attribute.\n", memreq_id);
      }

      va = get_attribute(memreq, "flags_prevent");
      vb = get_attribute(scene_memreq, "flags_prevent");
      if (strcmp(va, vb) != 0) {
        fprintf(stderr, "WARN: <memreq> for \"%s\" has odd \"flags_prevent\" attribute.\n", memreq_id);
      }

      va = get_attribute(memreq, "cpumap");
      vb = get_attribute(scene_memreq, "cpumap");
      if (strcmp(va, vb) != 0) {
        fprintf(stderr, "WARN: <memreq> for \"%s\" has odd \"cpumap\" attribute.\n", memreq_id);
      }
    }
  }
  sort_children(sctx, hypervisor);

  addr_space = query_element(sctx->doc, "/scenario/hypervisor/address_space");
  iterate_over_children(sctx->auxDoc, "/hypervisor/address_space", "map", map) //{
    const char *map_xref = get_attribute(map, "xref");
    xmlNodePtr scene_map = find_child_by_attribute(addr_space, NULL, "xref", map_xref);

    if (scene_map == NULL) {
      xmlNodePtr copied_map = xmlCopyNode(map, -1);
      xmlAddChild(addr_space, copied_map);

      fprintf(stderr, "INFO: copied over default <map> for \"%s\".\n", map_xref);
    }
  }
  sort_children(sctx, addr_space);

  return 0;
}

// ---------------------------------------------------------------------------

int main(int an, char **ac) {
  struct scenario_context sctx;
  xmlNodePtr arch, board, guest;
  uint64_t i;

  memset(&sctx, 0, sizeof(sctx));

  if (an != 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = SCENARIO_STAGE_PRISTINE;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  // ---------------------------------------------------------------------------

  // import <board>
  sctx.aux_category = "board";
  sctx.aux_name = query_attribute(sctx.doc, "/scenario/platform", "board");
  if (load_auxiliary(&sctx) != 0) {
    fprintf(stderr, "ERROR: Could not import <board> node: %s.\n", sctx.error_buffer);
    return 1;
  }
  board = xmlCopyNode(xmlDocGetRootElement(sctx.auxDoc), -1);
  xmlAddChild(query_element(sctx.doc, "/scenario/platform"), board);
  fprintf(stderr, "INFO: imported <board> node for \"%s\".\n", sctx.aux_name);

  // import <arch>
  sctx.aux_category = "arch";
  sctx.aux_name = query_attribute(sctx.doc, "/scenario/platform", "arch");
  if (load_auxiliary(&sctx) != 0) {
    fprintf(stderr, "ERROR: Could not import <arch> node: %s.\n", sctx.error_buffer);
    return 1;
  }
  arch = xmlCopyNode(xmlDocGetRootElement(sctx.auxDoc), -1);
  xmlAddChild(query_element(sctx.doc, "/scenario/platform"), arch);
  fprintf(stderr, "INFO: imported <arch> node for \"%s\".\n", sctx.aux_name);

  // ---------------------------------------------------------------------------

  // set "cpumap" attribute of <guest> nodes
  iterate_over_children(sctx.doc, "/scenario", "guest", guest) //{
    const char *guest_cpumap = get_attribute(guest, "cpumap");
    if (guest_cpumap == NULL) {
      unsigned long guest_ncpus = strtoul(get_attribute(guest, "ncpus"), NULL, 0);
      uint32_t *guest_cpumap_list = malloc(guest_ncpus * sizeof(int));
      for (i = 0; i < guest_ncpus; i++) {
        guest_cpumap_list[i] = i;
      }
      guest_cpumap = list_to_string(guest_cpumap_list, guest_ncpus);
      set_attribute(guest, "cpumap", guest_cpumap);
      fprintf(stderr, "INFO: created default cpumap attribute for guest \"%s\".\n",
		get_attribute(guest, "id"));
    } else {
      // check whether supplied list has correct size
    }
  }

  // ---------------------------------------------------------------------------

  // grab omitted <memreq> and <map> nodes from standard hypervisor
  if (populate_from_hypervisor(&sctx) != 0) {
    return 1;
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
