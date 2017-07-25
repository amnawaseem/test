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

int layout_memory_mmu(xmlNodePtr memory, uint32_t *aligns) {
  xmlNodePtr memreq;
  void *RM;
  const char *base_str;
  uint64_t base, size;
  int ret;

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

  /* step 2: allocate ranges ordered by request size
   *
   * The biggest requests go first. Each request is placed on the best
   * possible alignment constraint, so page table generation can later make
   * use of superpage/block descriptors, and overall TLB pressure is lower.
   */
  while (1) {
    uint64_t chosen_size;
    uint32_t align_index;
    xmlNodePtr *sizegroup;
    uint32_t sizegroup_cnt, sizegroup_len;
    uint32_t i;

    // determine size of largest unplaced <memreq> (if we get 0, we are done)
    chosen_size = 0;
    sizegroup = malloc(32 * sizeof(xmlNodePtr));
    sizegroup_cnt = 0;
    sizegroup_len = 32;

    iterate_over_children_N(memory, "memreq", memreq) //{
      base_str = get_attribute(memreq, "base");
      if (base_str != NULL)
        continue;

      size = strtoul(get_attribute(memreq, "size"), NULL, 0);
      if (size > chosen_size) {
        memset(sizegroup, 0, sizegroup_len * sizeof(xmlNodePtr));
        sizegroup_cnt = 0;
        sizegroup[sizegroup_cnt++] = memreq;
        chosen_size = size;
      } else if (size == chosen_size) {
        if (sizegroup_cnt == sizegroup_len) {
          sizegroup_len += 32;
          sizegroup = realloc(sizegroup, sizegroup_len * sizeof(xmlNodePtr));
          memset(sizegroup + (sizegroup_len-32), 0, 32 * sizeof(xmlNodePtr));
        }
        sizegroup[sizegroup_cnt++] = memreq;
      }
    }
    if (chosen_size == 0)
      break;

    // figure out optimal alignment shift for this size
    for (align_index = 0;
		aligns[align_index] && (chosen_size < (1UL << aligns[align_index]));
		align_index++)
	;
    if (aligns[align_index] == 0) {
      fprintf(stderr, "ERROR: no possible alignment for size 0x%lx.\n", size);
      return 1;
    }

    // allocate all open requests of this size
    for (i = 0; i < sizegroup_cnt; i++) {
      char *address_dict;
      uint32_t a;
      uint32_t custom_align;
      const char *cpumap;

      memreq = sizegroup[i];
      // fprintf(stderr, "Alloc %d/%d: \"%s\" (0x%lx)\n", i, sizegroup_cnt, get_attribute(memreq, "id"), chosen_size);
      custom_align = strtoul(get_attribute(memreq, "align_shift") ?: "", NULL, 0);
      cpumap = get_attribute(memreq, "cpumap");

      // dump_range_manager(RM);
      if (cpumap) {
        // it's a percpu or private resource; allocate as specified by cpumap, possibly multiple times
        uint32_t *cpumap_list, cpumap_len;
        uint32_t j;

        cpumap_list = string_to_list(cpumap, &cpumap_len);
        address_dict = malloc(cpumap_len * 32);
        address_dict[0] = '\0';
        for (j = 0; j < cpumap_len; j++) {
          a = align_index;
          ret = 1;
          while (ret && aligns[a]) {
            uint32_t try_align = (aligns[a] < custom_align) ? custom_align : aligns[a];
            ret = find_range_one_aligned(RM, 0, chosen_size, try_align, &base);
            if (ret) {
              fprintf(stderr, "INFO: Memory resource \"%s\" failed alignment test %u for CPU %u\n",
				get_attribute(memreq, "id"), try_align, cpumap_list[j]);
            }
            ++a;
          }
          if (ret) {
            fprintf(stderr, "ERROR: cannot place \"%s\".\n", get_attribute(memreq, "id"));
            return 1;
          }
          sprintf(address_dict + strlen(address_dict), "[%d]=0x%lx;", cpumap_list[j], base);
        }
        address_dict[strlen(address_dict) - 1] = '\0';
        set_attribute(memreq, "base", address_dict);
      } else {
        // it's a shared/global memory resource; allocate once
        a = align_index;
        ret = 1;
        while (ret && aligns[a]) {
          uint32_t try_align = (aligns[a] < custom_align) ? custom_align : aligns[a];
          ret = find_range_one_aligned(RM, 0, chosen_size, (aligns[a] < custom_align) ? custom_align : aligns[a], &base);
          if (ret) {
            fprintf(stderr, "INFO: Memory resource \"%s\" failed alignment test %u\n",
				get_attribute(memreq, "id"), try_align);
          }
          ++a;
        }
        if (ret) {
          fprintf(stderr, "ERROR: cannot place \"%s\".\n", get_attribute(memreq, "id"));
          return 1;
        }
        set_attribute_hexnumber(memreq, "base", base);
      }
    }

    free(sizegroup);
  }

  return 0;
}
