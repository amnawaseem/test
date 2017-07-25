#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <elf.h>
#include <schism/xml42.h>
#include <schism/elf.h>

uint64_t offset_pa_to_va;

uint64_t hypervisor_base;
uint64_t config_base;
uint64_t pagetables_base;
uint64_t blob_base;

// ---------------------------------------------------------------------------

static int check_phdr_alignment(struct scenario_context *sctx, Elf64_Phdr *phdr, unsigned int nphdr) {
  const char *hyp_as_format;
  xmlNodePtr format_node, level;
  uint32_t lowest_shift = 256;
  uint64_t align_mask = 0;
  unsigned int i;

  hyp_as_format = query_attribute(sctx->doc, "/scenario/hypervisor/address_space", "format");
  format_node = resolve(sctx->doc, hyp_as_format);

  if (strcmp((const char *)format_node->name, "paging_format") != 0) {
    return 0;
  }

  iterate_over_children_N(format_node, "level", level) //{
    uint32_t shift = strtoul(get_attribute(level, "shift"), NULL, 0);
    if (shift < lowest_shift)
      lowest_shift = shift;
  }

  align_mask = (1UL << lowest_shift) - 1;

  for (i = 0; i < nphdr; i++) {
    if (phdr[i].p_vaddr & align_mask) {
      fprintf(stderr, "ERROR: PHDR[%d] base unaligned: %lx & %lx\n",
				i, phdr[i].p_vaddr, align_mask);
      return 1;
    } else if (phdr[i].p_memsz & align_mask) {
      fprintf(stderr, "ERROR: PHDR[%d] size unaligned: %lx & %lx\n",
				i, phdr[i].p_memsz, align_mask);
      return 1;
    }
  }

  return 0;
}

// ---------------------------------------------------------------------------

static int _get_mm(struct scenario_context *sctx, const char *mmid, xmlNodePtr *memreq, xmlNodePtr *map) {
  xmlNodePtr hyp_as;

  *memreq = resolve(sctx->doc, mmid);

  if (*memreq == NULL) {
    fprintf(stderr, "ERROR: <memreq> node missing for \"%s\".\n", mmid);
    return 1;
  }

  hyp_as = query_element(sctx->doc, "/scenario/hypervisor/address_space");
  *map = find_child_by_attribute(hyp_as, "map", "xref", mmid);

  if (*map == NULL) {
    fprintf(stderr, "ERROR: <map> node missing for \"%s\".\n", mmid);
    return 1;
  }

  return 0;
}

// ---------------------------------------------------------------------------

static int measure_hypervisor_elf(struct scenario_context *sctx, const char *filename) {
  Elf64_Phdr *phdrs;
  xmlNodePtr hypervisor, memreq, map;
  uint64_t hypervisor_entry;

  hypervisor_entry = elf_entry_point(filename);

  phdrs = elf_to_phdrs(filename, 4);
  if (phdrs == NULL) {
    return 1;
  }
  if (check_phdr_alignment(sctx, phdrs, 4) != 0) {
    return 1;
  }

  hypervisor = query_element(sctx->doc, "/scenario/hypervisor");
  hypervisor_base = strtoul(get_attribute(hypervisor, "load_base"), NULL, 0);

  offset_pa_to_va = phdrs[0].p_vaddr - hypervisor_base;
  config_base = hypervisor_base + (phdrs[3].p_vaddr - phdrs[0].p_vaddr) + phdrs[3].p_memsz;

  if (_get_mm(sctx, "core_rx", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "core_rx");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", hypervisor_base);
  set_attribute_hexnumber(memreq, "size", phdrs[0].p_memsz);
  set_attribute_hexnumber(map, "base", phdrs[0].p_vaddr);

  if (_get_mm(sctx, "core_r", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "core_r");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", hypervisor_base + (phdrs[1].p_vaddr - phdrs[0].p_vaddr));
  set_attribute_hexnumber(memreq, "size", phdrs[1].p_memsz);
  set_attribute_hexnumber(map, "base", phdrs[1].p_vaddr);

  if (_get_mm(sctx, "core_rws", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "core_rws");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", hypervisor_base + (phdrs[2].p_vaddr - phdrs[0].p_vaddr));
  set_attribute_hexnumber(memreq, "size", phdrs[2].p_memsz);
  set_attribute_hexnumber(map, "base", phdrs[2].p_vaddr);

  if (_get_mm(sctx, "core_rwt", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "core_rwt");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", hypervisor_base + (phdrs[3].p_vaddr - phdrs[0].p_vaddr));
  set_attribute_hexnumber(memreq, "size", phdrs[3].p_memsz);
  if (get_attribute(map, "base") != NULL) {
    fprintf(stderr, "ERROR: <map> node for \"%s\" has preset base address.\n", "core_rwt");
    return 1;
  }

  if (_get_mm(sctx, "core_rw", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "core_rw");
    return 1;
  }
  if (get_attribute(memreq, "base") != NULL) {
    fprintf(stderr, "ERROR: <memreq> node for \"%s\" has preset base address.\n", "core_rw");
    return 1;
  }
  set_attribute_hexnumber(memreq, "size", phdrs[3].p_memsz);
  set_attribute_hexnumber(map, "base", phdrs[3].p_vaddr);

  /* convert virtual hypervisor entry to physical using offset calculated above */

  set_attribute_hexnumber(hypervisor, "entry", hypervisor_entry - offset_pa_to_va);

  return 0;
}

static int measure_config_data(struct scenario_context *sctx, const char *filename) {
  Elf64_Phdr *phdrs;
  xmlNodePtr memreq, map;

  phdrs = elf_to_phdrs(filename, 3);
  if (phdrs == NULL) {
    return 1;
  }
  if (check_phdr_alignment(sctx, phdrs, 3) != 0) {
    return 1;
  }

  pagetables_base = config_base + (phdrs[2].p_vaddr - phdrs[0].p_vaddr) + phdrs[2].p_memsz;

  if (_get_mm(sctx, "config_r", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "config_r");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", config_base);
  set_attribute_hexnumber(memreq, "size", phdrs[0].p_memsz);
  set_attribute_hexnumber(map, "base", offset_pa_to_va + config_base);

  if (_get_mm(sctx, "config_rw", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "config_rw");
    return 1;
  }
  if (get_attribute(memreq, "base") != NULL) {
    fprintf(stderr, "ERROR: <memreq> node for \"%s\" has preset base address.\n", "config_rw");
    return 1;
  }
  set_attribute_hexnumber(memreq, "size", phdrs[1].p_memsz);
  set_attribute_hexnumber(map, "base", offset_pa_to_va + config_base + (phdrs[1].p_vaddr - phdrs[0].p_vaddr));

  if (_get_mm(sctx, "config_rws", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "config_rws");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", config_base + (phdrs[2].p_vaddr - phdrs[0].p_vaddr));
  set_attribute_hexnumber(memreq, "size", phdrs[2].p_memsz);
  set_attribute_hexnumber(map, "base", offset_pa_to_va + config_base + (phdrs[2].p_vaddr - phdrs[0].p_vaddr));

  if (_get_mm(sctx, "config_rwt", &memreq, &map) != 0) {
    fprintf(stderr, "ERROR: <memreq>/<map> node missing for \"%s\".\n", "config_rwt");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", config_base + (phdrs[1].p_vaddr - phdrs[0].p_vaddr));
  set_attribute_hexnumber(memreq, "size", phdrs[1].p_memsz);
  if (get_attribute(map, "base") != NULL) {
    fprintf(stderr, "ERROR: <map> node for \"%s\" has preset base address.\n", "config_rwt");
    return 1;
  }

  return 0;
}

static uint64_t get_default_estimate(struct scenario_context *sctx, const char *format_name) {
  xmlNodePtr format;
  const char *format_size_estimate;

  format = resolve(sctx->doc, format_name);

  if (format == NULL) {
    fprintf(stderr, "ERROR: paging/protection format not found, cannot estimate size, using 0.\n");
    return 0;
  }

  format_size_estimate = get_attribute(format, "size_estimate");
  if (format_size_estimate == NULL) {
    fprintf(stderr, "ERROR: no default estimate value, using 0.\n");
    return 0;
  }

  return strtoul(format_size_estimate, NULL, 0);
}

static int estimate_pagetables(struct scenario_context *sctx) {
  xmlNodePtr guest, as, memreq;
  const char *as_estimate, *as_format;
  uint64_t hyp_ncpus = 0;
  uint64_t guest_ncpus = 0;
  uint64_t size_estimate = 0;
  uint64_t estimate_item = 0;

  as = query_element(sctx->doc, "/scenario/hypervisor/address_space");

  as_estimate = get_attribute(as, "size_estimate");
  as_format = get_attribute(as, "format");

  if (as_estimate) {
    estimate_item = strtoul(as_estimate, NULL, 0);
  } else {
    estimate_item = get_default_estimate(sctx, as_format);
  }
  hyp_ncpus = strtoul(query_attribute(sctx->doc, "/scenario/hypervisor", "ncpus"), NULL, 0);
  size_estimate += (hyp_ncpus + 1) * estimate_item;

  iterate_over_children(sctx->doc, "/scenario", "guest", guest) //{
    as = get_child(guest, "address_space");
    as_estimate = get_attribute(as, "size_estimate");
    as_format = get_attribute(as, "format");

    if (strcmp(as_format, "none") == 0) continue;
    if (as_estimate) {
      estimate_item = strtoul(as_estimate, NULL, 0);
    } else {
      estimate_item = get_default_estimate(sctx, as_format);
    }
    guest_ncpus = strtoul(get_attribute(guest, "ncpus"), NULL, 0);
    size_estimate += guest_ncpus * estimate_item;
  }

  memreq = resolve(sctx->doc, "pagetables");
  if (memreq == NULL) {
    fprintf(stderr, "ERROR: <memreq> node missing for pagetable data.\n");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", pagetables_base);
  set_attribute_hexnumber(memreq, "size", size_estimate);

  fprintf(stderr, "INFO: estimating total size of pagetables: 0x%lx\n", size_estimate);

  blob_base = pagetables_base + size_estimate;

  return 0;
}

static int measure_file_blob(struct scenario_context *sctx) {
  struct stat ffs;
  unsigned int fsz;
  xmlNodePtr file, memreq;

  fsz = 0;
  iterate_over_children(sctx->doc, "/scenario/files", "file", file) //{
    const char *filename = get_attribute(file, "href");
    if (stat(filename, &ffs) != 0) {
      fprintf(stderr, "ERROR: cannot access file \"%s\".\n", filename);
      return 1;
    }
    set_attribute_hexnumber(file, "offset", fsz);
    set_attribute_hexnumber(file, "size", ffs.st_size);

    fsz += ffs.st_size;
  }
  if (fsz & 0xfff)
    fsz = (fsz & ~0xfff) + 0x1000;

  memreq = resolve(sctx->doc, "blob");
  if (memreq == NULL) {
    fprintf(stderr, "ERROR: <memreq> node missing for file blob.\n");
    return 1;
  }
  set_attribute_hexnumber(memreq, "base", blob_base);
  set_attribute_hexnumber(memreq, "size", fsz);

  fprintf(stderr, "INFO: file blob has size 0x%x.\n", fsz);

  return 0;
}

// ---------------------------------------------------------------------------

int main(int an, char **ac) {
  struct scenario_context sctx;
  char fn[256];

  memset(&sctx, 0, sizeof(sctx));

  if (an != 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = SCENARIO_STAGE_REPARENTED;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  // ---------------------------------------------------------------------------

  // measure hypervisor ELF sizes
  snprintf(fn, 256, "%s/%s.elf", ac[1], query_attribute(sctx.doc, "/scenario", "cbi"));
  if (measure_hypervisor_elf(&sctx, fn)) return 1;

  // measure config data
  snprintf(fn, 256, "%s/scenario_config.xo", ac[1]);
  if (measure_config_data(&sctx, fn)) return 1;

  // estimate pagetable size
  if (estimate_pagetables(&sctx)) return 1;

  // measure file blob size
  if (measure_file_blob(&sctx)) return 1;

  // ---------------------------------------------------------------------------

  sctx.stage++;
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate transformed scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  save_scenario(&sctx);

  return 0;
}
