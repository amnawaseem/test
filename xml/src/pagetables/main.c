#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <elf.h>
#include <schism/xml42.h>
#include <schism/elf.h>
#include "pagetables.h"

uint64_t pagetable_base;
int verbose_mode;

// ---------------------------------------------------------------------------

int main(int an, char **ac) {
  struct scenario_context sctx;
  char fn[256];
  xmlNodePtr memreq, address_space, guest;
  uint32_t *guest_cpumap, guest_cpumap_len;
  const char *as_type;
  uint64_t pagetable_actual_size;
  uint64_t pagetable_estimated_size;
  int pagetable_fd;
  uint32_t ncpus, j;

  memset(&sctx, 0, sizeof(sctx));

  if (an != 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = SCENARIO_STAGE_VIRTUAL_LAID_OUT;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  // verbose_mode = 1;

  // ---------------------------------------------------------------------------

  memreq = resolve(sctx.doc, "pagetables");
  if (memreq == NULL) {
    fprintf(stderr, "ERROR: no pagetables <memreq>.\n");
    return 1;
  }

  pagetable_base = strtoul(get_attribute(memreq, "base"), NULL, 0);
  pagetable_estimated_size = strtoul(get_attribute(memreq, "size"), NULL, 0);

  // ---------------------------------------------------------------------------

  sprintf(fn, "%s/pagetables", ac[1]);
  pagetable_fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  address_space = query_element(sctx.doc, "/scenario/hypervisor/address_space");
  as_type = get_attribute(address_space, "type");
  ncpus = strtoul(query_attribute(sctx.doc, "/scenario/hypervisor", "ncpus"), NULL, 0);

  if (strcmp(as_type, "mmu") == 0) {
    if (create_pagetable(&sctx, address_space, -1, -1, pagetable_fd)) return 1;

    for (j = 0; j < ncpus; j++) {
      if (create_pagetable(&sctx, address_space, j, j, pagetable_fd)) return 1;
    }

    iterate_over_children(sctx.doc, "/scenario", "guest", guest) //{
      address_space = get_child(guest, "address_space");
      as_type = get_attribute(address_space, "type");
      ncpus = strtoul(get_attribute(guest, "ncpus"), NULL, 0);
      guest_cpumap = string_to_list(get_attribute(guest, "cpumap"), &guest_cpumap_len);

      if (strcmp(as_type, "mmu") != 0) {
        fprintf(stderr, "ERROR: only mmu or vtlb guests supported on mmu hypervisors\n");
        return 1;
      }

      for (j = 0; j < ncpus; j++) {
        uint32_t host_cpu = guest_cpumap[j];
        if (create_pagetable(&sctx, address_space, j, host_cpu, pagetable_fd)) return 1;
      }
    }
  } else if (strcmp(as_type, "mpu") == 0) {
    // no INIT address space; MPU can be turned on without intermediate step
    for (j = 0; j < ncpus; j++) {
      if (create_protections(&sctx, address_space, j, j, pagetable_fd)) return 1;
    }

    // TODO: add support for guests
  } else if (strcmp(as_type, "flat") == 0) {
    create_flat(&sctx);
  } else {
    fprintf(stderr, "ERROR: unknown address space type.\n");
    return 1;
  }

  pagetable_actual_size = lseek(pagetable_fd, 0, SEEK_END);

  if (pagetable_actual_size > pagetable_estimated_size) {
    fprintf(stderr, "ERROR: total size of pagetables exceeds estimate (0x%lx > 0x%lx).\n",
		pagetable_actual_size, pagetable_estimated_size);
    return 1;
  } else {
    fprintf(stderr, "INFO: total size of pagetables within estimate (0x%lx <= 0x%lx).\n",
		pagetable_actual_size, pagetable_estimated_size);
  }

  lseek(pagetable_fd, pagetable_estimated_size, SEEK_SET);
  close(pagetable_fd);

  // ---------------------------------------------------------------------------

  sctx.stage++;
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate transformed scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  save_scenario(&sctx);

  return 0;
}
