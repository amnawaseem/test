#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>
#include <schism/range.h>
#include "pagetables.h"

// ---------------------------------------------------------------------------

/**
 * Read a full paging format specification from XML into internal data
 * structures.
 * The parser populates the global "paging_levels" array and the
 * "paging_num_levels" counter.
 */
static int parse_paging_format(xmlNodePtr paging_format) {
  uint64_t i, j;
  xmlNodePtr level, flag, flaggroup;

  paging_num_levels = count_children(paging_format, "level");
  paging_levels = malloc(paging_num_levels * sizeof(struct paging_level));
  memset(paging_levels, 0, paging_num_levels * sizeof(struct paging_level));

  i = 0;
  iterate_over_children_N(paging_format, "level", level) //{
    paging_levels[i].shift = get_attribute_number(level, "shift");
    paging_levels[i].width = get_attribute_number(level, "width");
    paging_levels[i].bits_per_entry = get_attribute_number(level, "bpe");
    paging_levels[i].alignment_shift = get_attribute_number(level, "align");

    if (get_attribute(level, "dir_base") != NULL) {
      paging_levels[i].can_dir = 1;
      paging_levels[i].dir_entry = get_attribute_number(level, "dir_base");
    }
    if (get_attribute(level, "leaf_base") != NULL) {
      paging_levels[i].can_leaf = 1;
      paging_levels[i].leaf_entry = get_attribute_number(level, "leaf_base");
    }

    iterate_over_children_N(level, "flag", flag) //{
      char flag_str = get_attribute(flag, "name")[0];
      enum permission_index index;
      switch (flag_str) {
      case 'r': index = PERM_R; break;
      case 'w': index = PERM_W; break;
      case 'x': index = PERM_X; break;
      case 'g': index = PERM_G; break;
      case 'd': index = PERM_D; break;
      case 'u': index = PERM_U; break;
      case 's': index = PERM_S; break;
      default:
        fprintf(stderr, "ERROR: unknown paging flag.\n");
        return 1;
      }

      paging_levels[i].permission_flags[index] = get_attribute_number(flag, "value_set");
      paging_levels[i].permission_flags[index+1] = get_attribute_number(flag, "value_clear");
    }

    paging_levels[i].num_flaggroups = count_children(level, "flaggroup");
    if (paging_levels[i].num_flaggroups > 0) {
      paging_levels[i].flaggroups = malloc(sizeof(struct flaggroup) * paging_levels[i].num_flaggroups);
      j = 0;
      iterate_over_children_N(level, "flaggroup", flaggroup) //{
        paging_levels[i].flaggroups[j].flaggroup = get_attribute(flaggroup, "group");
        paging_levels[i].flaggroups[j].bits_set = get_attribute_number(flaggroup, "bits_set");
        paging_levels[i].flaggroups[j].bits_clear = get_attribute_number(flaggroup, "bits_clear");
        ++j;
      }
    }

    if ((i > 0) && (paging_levels[i].shift > paging_levels[i-1].shift)) {
      fprintf(stderr, "ERROR: paging format levels not in descending order.\n");
      return 1;
    }

    ++i;
  }

  return 0;
}

// ---------------------------------------------------------------------------

/**
 * Convert a list of permission flags into the corresponding pagetable
 * entry bit combination. As the area may be split across different
 * levels (e.g. some level2 entries, then some big level1 ones, then again
 * some small level2), bit combinations are computed for all levels in
 * advance.
 */
static uint64_t *assemble_flag_bits(const char *flags) {
  uint64_t *flag_bits;
  uint32_t i;

  flag_bits = malloc(paging_num_levels * sizeof(uint64_t));

  for (i = 0; i < paging_num_levels; i++) {
    flag_bits[i] = paging_levels[i].leaf_entry;
    flag_bits[i] |= paging_levels[i].permission_flags[PERM_R + (strchr(flags, 'r') ? 0 : 1)];
    flag_bits[i] |= paging_levels[i].permission_flags[PERM_W + (strchr(flags, 'w') ? 0 : 1)];
    flag_bits[i] |= paging_levels[i].permission_flags[PERM_X + (strchr(flags, 'x') ? 0 : 1)];
    flag_bits[i] |= paging_levels[i].permission_flags[PERM_G + (strchr(flags, 'g') ? 0 : 1)];
    flag_bits[i] |= paging_levels[i].permission_flags[PERM_D + (strchr(flags, 'd') ? 0 : 1)];
    flag_bits[i] |= paging_levels[i].permission_flags[PERM_U + (strchr(flags, 'u') ? 0 : 1)];
    flag_bits[i] |= paging_levels[i].permission_flags[PERM_S + (strchr(flags, 's') ? 0 : 1)];

    if (paging_levels[i].num_flaggroups != 0) {
      fprintf(stderr, "ERROR: implement me!\n");
      return NULL;
    }
  }

  return flag_bits;
}

// ---------------------------------------------------------------------------

/**
 * Allocate a new pagetable directory on the specified level.
 * The new directory is appended to the "unplaced_dir_list", i.e. no physical
 * address is assigned at this point.
 */
static struct pagetable_dir *alloc_directory(int level) {
  struct pagetable_dir *d;

  d = malloc(sizeof(struct pagetable_dir));
  d->level = level;
  d->is_placed = 0;
  d->phys_addr = 0;
  d->entries = malloc((1 << paging_levels[level].width) * sizeof(struct pagetable_entry));
  memset(d->entries, 0, (1 << paging_levels[level].width) * sizeof(struct pagetable_entry));

  unplaced_dir_list = realloc(unplaced_dir_list, (unplaced_dirs + 1) * sizeof(void *));
  unplaced_dir_list[unplaced_dirs] = d;
  unplaced_dirs++;

  return d;
}

/**
 * Add the given mapping [va,size] --(flags)--> [pa,size] to the pagetable.
 */
static int add_mapping(uint64_t va, uint64_t pa, uint64_t size, const char *flags) {
  struct pagetable_dir *d = pagetable_tree;
  uint64_t *flag_bits;

  flag_bits = assemble_flag_bits(flags);

  verbose_printf("PTBL +m %lx -> %lx +%lx\n", va, pa, size);

  while (size) {
    struct pagetable_entry *e;
    int va_index;

    va_index = (va >> paging_levels[d->level].shift) & ALIGN_MASK(paging_levels[d->level].width);
    e = d->entries + va_index;

    if (	(!paging_levels[d->level].can_leaf) ||
		(va & ALIGN_MASK(paging_levels[d->level].shift)) ||
    		(pa & ALIGN_MASK(paging_levels[d->level].shift)) ||
    		(size < ALIGN_MASK(paging_levels[d->level].shift))	) {
      if (e->entry_type == ENTRY_DIR) {
        d = e->dir;
      } else if (e->entry_type == 0) {
        verbose_printf("PTBL +d(%d) %lx+%lx\n", d->level, va, size);
        e->entry_type = ENTRY_DIR;
        d = e->dir = alloc_directory(d->level + 1);
      } else {
        fprintf(stderr, "Clash, entry present where dir should be at 0x%lx\n", va);
        return 1;
      }
    } else {
      if (e->entry_type) {
        fprintf(stderr, "Clash, entry present at 0x%lx\n", va);
        return 1;
      }
      verbose_printf("PTBL +e(%d) %lx+%lx\n", d->level, va, size);
      e->entry_type = ENTRY_LEAF;
      e->dir = NULL;
      e->page_addr = pa | flag_bits[d->level];

      va += (1UL << paging_levels[d->level].shift);
      pa += (1UL << paging_levels[d->level].shift);
      size -= (1UL << paging_levels[d->level].shift);
      d = pagetable_tree;
    }
  }

  free(flag_bits);

  return 0;
}

/**
 * cpu_no: the dictionary index the base address of this pagetable will be recorded as
 *         (in the "base" attribute of the <mmu> node)
 */
static int place_directories(xmlNodePtr mmu, int cpu_no, int fd) {
  uint32_t i, j;
  int align;
  uint64_t allocation_base = pagetable_base;
  struct pagetable_dir **placed_dir_list = malloc(unplaced_dirs * sizeof(void *));
  uint32_t placed_dirs = 0;

  while (1) {
    align = 0;
    for (i = 0; i < unplaced_dirs; i++) {
      if (unplaced_dir_list[i]->is_placed) continue;
      if (paging_levels[unplaced_dir_list[i]->level].alignment_shift < align) continue;
      align = paging_levels[unplaced_dir_list[i]->level].alignment_shift;
    }
    if (align == 0) break;

    for (i = 0; i < unplaced_dirs; i++) {
      struct pagetable_dir *d = unplaced_dir_list[i];

      if (d->is_placed) continue;
      if (paging_levels[d->level].alignment_shift != align) continue;

      d->phys_addr = allocation_base;
      if (d->phys_addr & ALIGN_MASK(align)) {
        d->phys_addr = (d->phys_addr & ~ALIGN_MASK(align)) + (1UL << align);
      }
      d->head_padding = d->phys_addr - allocation_base;
      allocation_base = d->phys_addr + \
			(paging_levels[d->level].bits_per_entry \
			<< paging_levels[d->level].width);
      d->is_placed = 1;
      placed_dir_list[placed_dirs] = d;
      placed_dirs++;

      if (d->level == 0) {
        dict_append_hexnumber(mmu, "base", cpu_no, d->phys_addr);
      } else {
        d->phys_addr |= paging_levels[d->level - 1].dir_entry;
      }
    }
  }

  for (i = 0; i < placed_dirs; i++) {
    struct pagetable_dir *d = placed_dir_list[i];

    (void)lseek(fd, d->head_padding, SEEK_CUR);
    for (j = 0; j < (1U << paging_levels[d->level].width); j++) {
      uint64_t val;

      switch (d->entries[j].entry_type) {
      case 0:		val = 0; break;
      case ENTRY_LEAF:	val = d->entries[j].page_addr; break;
      case ENTRY_DIR:	val = d->entries[j].dir->phys_addr; break;
      }

      switch (paging_levels[d->level].bits_per_entry) {
      case 4:		write(fd, (void *)&val, 4); break;
      case 8:		write(fd, (void *)&val, 8); break;
      }
    }
  }

  pagetable_base = allocation_base;

  return 0;
}

// ---------------------------------------------------------------------------

/**
 * Build up the pagetable for a given <mmu> node.
 * Case 1: <hypervisor><mmu>
 *   host_cpu: -1 (INIT pagetable) or [0..ncpu-1] (core_space)
 *   mmu_cpu: == host_cpu
 * Case 2: <guest><mmu>
 *   mmu_cpu: [0..guest_ncpu-1] (virt_space)
 *   host_cpu: physical CPU this virt_space is used on
 */
int create_pagetable(struct scenario_context *sctx, xmlNodePtr mmu, int mmu_cpu, int host_cpu, int fd) {
  xmlNodePtr paging_format, map, xref;
  const char *xref_base_str, *va_str;
  uint64_t va, pa, size;

  paging_format = resolve(sctx->doc, get_attribute(mmu, "format"));
  if (paging_format == NULL) {
    fprintf(stderr, "ERROR: unknown paging format \"%s\".\n", get_attribute(mmu, "format"));
    return 1;
  }
  if (parse_paging_format(paging_format)) {
    return 1;
  }

  if (get_attribute(mmu, "base") == NULL)
    set_attribute(mmu, "base", "");

  unplaced_dirs = 0;
  unplaced_dir_list = NULL;

  verbose_printf("Building pagetable for CPU %d/%d\n", mmu_cpu, host_cpu);

  pagetable_tree = alloc_directory(0);

  iterate_over_children_N(mmu, "map", map) //{

    if ((host_cpu < 0) && (get_attribute(map, "is_init") == NULL))
      continue;

    if ((get_attribute(map, "cpumap") != NULL) &&
		(!list_contains(map, "cpumap", mmu_cpu)))
      continue;

    xref = resolve(sctx->doc, get_attribute(map, "xref"));
    if (xref == NULL) {
      fprintf(stderr, "ERROR: no <memreq> or <device> for <map> \"%s\".\n", get_attribute(map, "xref"));
      return 1;
    }

    if ((get_attribute(xref, "cpumap") != NULL) &&
		(!list_contains(xref, "cpumap", host_cpu)))
      continue;

    size = get_attribute_number(xref, "size");
    xref_base_str = get_attribute(xref, "base");
    if (xref_base_str[0] == '[') {
      pa = dictS_get_hexnumber(xref_base_str, host_cpu);
    } else {
      pa = strtoul(xref_base_str, NULL, 0);
    }

    if (get_attribute(map, "offset") != NULL) {
      pa += get_attribute_number(map, "offset");
      // DO NOT INCREMENT va
      size -= get_attribute_number(map, "offset");
    }
    if (get_attribute(map, "subsize") != NULL) {
      size = get_attribute_number(map, "subsize");
    }

    va_str = get_attribute(map, "base");
    if (va_str[0] == '[') {
      va = dictS_get_hexnumber(va_str, mmu_cpu);
    } else {
      va = strtoul(va_str, NULL, 0);
    }

    if ((host_cpu < 0) && (va != pa)) {
      verbose_printf("PTBL +map %s (INIT 1:1)\n", get_attribute(map, "xref"));
      if (add_mapping(pa, pa, size, get_attribute(map, "flags"))) {
        fprintf(stderr, "ERROR: map insertion (INIT 1:1) failed for \"%s\".\n", get_attribute(map, "xref"));
        return 1;
      }
    }

    verbose_printf("PTBL +map %s\n", get_attribute(map, "xref"));
    if (add_mapping(va, pa, size, get_attribute(map, "flags"))) {
      fprintf(stderr, "ERROR: map insertion failed for \"%s\".\n", get_attribute(map, "xref"));
      return 1;
    }
  }

  if (place_directories(mmu, mmu_cpu, fd)) return 1;

  return 0;
}
