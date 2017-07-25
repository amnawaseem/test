#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>
#include <schism/range.h>
#include "layouter.h"

// ---------------------------------------------------------------------------

static int layout_map_mmu_fixmaps(void *RM, struct scenario_context *sctx, xmlNodePtr address_space) {
  const char *base_str, *size_str;
  uint64_t base_tmp, size_tmp;
  xmlNodePtr map, xref;

  iterate_over_children_N(address_space, "map", map) //{
    base_str = get_attribute(map, "base");
    if (base_str == NULL)
      continue;

    if (base_str[0] == '[') {
      fprintf(stderr, "ERROR: cannot claim pre-placed percore <map> nodes yet.\n");
      return 1;
    }
    base_tmp = strtoul(get_attribute(map, "base"), NULL, 0);

    xref = resolve(sctx->doc, get_attribute(map, "xref"));
    if (xref == NULL) {
      fprintf(stderr, "ERROR: no reference node for <map> node \"%s\".\n", get_attribute(map, "xref"));
      return 1;
    }

    size_str = get_attribute(map, "subsize");
    if (size_str == NULL)
      size_str = get_attribute(xref, "size");
    size_tmp = strtoul(size_str, NULL, 0);

    if (claim_range_all(RM, base_tmp, size_tmp) != 0) {
      fprintf(stderr, "ERROR: cannot claim \"%s\".\n", get_attribute(map, "xref"));
      return 1;
    }
    // fprintf(stderr, "DEBUG: Claimed %s, size 0x%lx\n", get_attribute(map, "xref"), size_tmp);
  }

  return 0;
}

static int layout_map_mmu_others(void *RM, struct scenario_context *sctx, xmlNodePtr address_space,
				uint32_t *aligns, int do_common) {
  uint64_t i;
  xmlNodePtr map, xref;
  const char *base_str, *size_str, *cpumap;
  char *new_base_str;
  uint32_t *cpumap_list, cpumap_len;
  uint64_t base, size, size_tmp;
  xmlNodePtr *sizegroup;
  uint32_t sizegroup_cnt, sizegroup_len;
  uint32_t align_index, j, k;
  uint64_t window[2];
  int ret;

  window[0] = strtoul(query_attribute(sctx->doc, "/scenario/hypervisor", "exec_base"), NULL, 0);
  window[1] = 0xffffffffffffffffUL;

  while (1) {
    size = 0;
    sizegroup = malloc(32 * sizeof(xmlNodePtr));
    sizegroup_cnt = 0;
    sizegroup_len = 32;

    // determine <map> set with largest size
    iterate_over_children_N(address_space, "map", map) //{
      base_str = get_attribute(map, "base");
      if (base_str != NULL)
        continue;

      cpumap = get_attribute(map, "cpumap");
      if (do_common && (cpumap != NULL)) {
        continue;
      } else if (!do_common && (cpumap == NULL)) {
        continue;
      }

      xref = resolve(sctx->doc, get_attribute(map, "xref"));

      size_str = get_attribute(map, "subsize");
      if (size_str == NULL)
        size_str = get_attribute(xref, "size");
      size_tmp = strtoul(size_str, NULL, 0);

      if (size_tmp > size) {
        memset(sizegroup, 0, sizegroup_len * sizeof(xmlNodePtr));
        sizegroup_cnt = 0;
        sizegroup[sizegroup_cnt++] = map;
        size = size_tmp;
      } else if (size_tmp == size) {
        if (sizegroup_cnt == sizegroup_len) {
          sizegroup_len += 32;
          sizegroup = realloc(sizegroup, sizegroup_len * sizeof(xmlNodePtr));
          memset(sizegroup + (sizegroup_len-32), 0, 32 * sizeof(xmlNodePtr));
        }
        sizegroup[sizegroup_cnt++] = map;
      }
    }
    if (size == 0) break;

    for (i = 0; aligns[i] && (size < (1UL << aligns[i])); i++)
      ;
    if (aligns[i] == 0) {
      fprintf(stderr, "ERROR: size of <map> \"%s\" is not minimally aligned.\n", get_attribute(map, "xref"));
      return 1;
    }
    align_index = i;

    for (i = 0; i < sizegroup_cnt; i++) {
      uint64_t *use_window;

      map = sizegroup[i];
      if (strchr(get_attribute(map, "flags"), 'g') != NULL) {
        use_window = window;
      } else {
        use_window = NULL;
      }

      xref = resolve(sctx->doc, get_attribute(map, "xref"));
      cpumap = get_attribute(map, "cpumap");

      j = align_index;
      ret = 1;

      if (!do_common) {
        cpumap_list = string_to_list(cpumap, &cpumap_len);
        new_base_str = malloc(32 * cpumap_len);
        new_base_str[0] = '\0';

        for (k = 0; k < cpumap_len; k++) {
          while (ret && aligns[j]) {
            ret = find_range_one_aligned_padded_window(RM, cpumap_list[k], size, aligns[j], 12, use_window, &base);
            if (ret) {
              fprintf(stderr, "INFO: Map \"%s\" failed CPU%d alignment test %u\n",
                                get_attribute(map, "xref"), cpumap_list[k], aligns[j]);
            }
            ++j;
          }
          if (ret) {
            fprintf(stderr, "ERROR: cannot place \"%s\".\n", get_attribute(map, "xref"));
            return 1;
          }
          sprintf(new_base_str + strlen(new_base_str), "[%d]=0x%lx;", cpumap_list[k], base);
        }

        new_base_str[strlen(new_base_str)-1] = '\0';
        set_attribute(map, "base", new_base_str);
      } else {
        while (ret && aligns[j]) {
          ret = find_range_all_aligned_padded_window(RM, size, aligns[j], 12, use_window, &base);
          if (ret) {
            fprintf(stderr, "INFO: Map \"%s\" failed alignment test %u\n",
                                get_attribute(map, "xref"), aligns[j]);
          }
          ++j;
        }
        if (ret) {
          fprintf(stderr, "ERROR: cannot place \"%s\".\n", get_attribute(map, "xref"));
          return 1;
        }

        set_attribute_hexnumber(map, "base", base);
      }
    }

    free(sizegroup);
  }

  return 0;
}

// ---------------------------------------------------------------------------

int layout_map_mmu(struct scenario_context *sctx, xmlNodePtr address_space) {
  xmlNodePtr arch, hyp_pg_format;
  uint32_t *aligns;
  void *RM;
  uint64_t i, ncpus;
  uint32_t cnt;

  /* support for paravirt (globals_window[]) dropped */

  arch = query_element(sctx->doc, "/scenario/platform/arch");
  hyp_pg_format = find_child_by_attribute(arch, "paging_format", "id",
			get_attribute(address_space, "format"));
  ncpus = strtoul(query_attribute(sctx->doc, "/scenario/hypervisor", "ncpus"), NULL, 0);

  aligns = mmu_gather_alignments(sctx, hyp_pg_format);
  for (i = 0; aligns[i]; i++)
    ;
  cnt = i;

  fprintf(stderr, "INFO: virtual layout for MMU, alignments: %s\n", list_to_string(aligns, cnt));

  RM = init_range_manager(1UL << aligns[i-1], 0UL - (2UL << aligns[i-1]), ncpus);

  layout_map_mmu_fixmaps(RM, sctx, address_space);
  layout_map_mmu_others(RM, sctx, address_space, aligns, 1);
  layout_map_mmu_others(RM, sctx, address_space, aligns, 0);

  return 0;
}
