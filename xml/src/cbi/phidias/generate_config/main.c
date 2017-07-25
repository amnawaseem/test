#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

const char *phidias_addendum_preamble = \
	"#include <phidias.h>\n"
	"#include <vm.h>\n"
	"#include <interrupts.h>\n"
	"#include <emulate/core.h>\n"
	"#include <emulate/uart.h>\n"
	"#include <emulate/irq.h>\n"
	"#include <emulate/timer.h>\n"
	"#include <emulate/clock.h>\n"
	"#include <emulate/vtlb.h>\n"
	"#include <emulate/memory.h>\n"
	"#include <arch/cpu_state.h>\n"
	"#include <schedule.h>\n"
	"#include <specification.h>\n"
	"#include <arch/specification.h>\n";

static struct addendum_section phidias_addendum_sections[] = {
	{ ".rodata_head",	"__SEC_ROH",	ADDENDUM_SECTION_FLAG_RO },
	{ ".rodata",		"__SEC_RO",	ADDENDUM_SECTION_FLAG_RO },
	{ ".data",		"__SEC_RW",	0 },
	{ ".data_shared",	"__SEC_RWS",	0 },
};

#define	ADDENDUM_SEC_COUNT	4

// ---------------------------------------------------------------------------

uint64_t base_address = 0UL;

static int lookup_emit_base_address(struct scenario_context *sctx) {
  xmlNodePtr map;
  const char *base_address_str;
  char fn[256];
  int fd;

  map = find_child_by_attribute(
		query_element(sctx->doc, "/scenario/hypervisor/address_space"),
		"map", "xref", "config_r");
  if (map == NULL) {
    return 1;
  }
  base_address_str = get_attribute(map, "base");
  if (base_address_str == NULL) {
    return 1;
  }

  snprintf(fn, 256, "%s/config_base.lds", sctx->builddir);
  fd = open(fn, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd < 0) {
    fprintf(stderr, "ERROR: could not open output file.\n");
    return 1;
  }
  write(fd, "CONFIG_START = ", 15);
  write(fd, base_address_str, strlen(base_address_str));
  write(fd, ";\n", 2);
  close(fd);

  base_address = strtoul(base_address_str, NULL, 0);

  return 0;
}

// ---------------------------------------------------------------------------

int main(int an, char **ac) {
  struct scenario_context sctx;
  int stage;
  struct addendum *A;

  if (an != 3) {
    fprintf(stderr, "ERROR: need build directory argument and iteration number.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  stage = atoi(ac[2]);

  switch (stage) {
  case 1:
	sctx.stage = SCENARIO_STAGE_REPARENTED;
	break;
  case 2:
	sctx.stage = SCENARIO_STAGE_PAGETABLES;
	break;
  default:
	fprintf(stderr, "ERROR: invalid iteration number.\n");
	return 1;
  }

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  if ((stage == 2) && (lookup_emit_base_address(&sctx) != 0)) {
    return 1;
  }

  A = init_addendum(phidias_addendum_sections, ADDENDUM_SEC_COUNT);
  A->preamble = phidias_addendum_preamble;

  if (load_reflections(&sctx, A, stage)) {
    return 1;
  }

  if (generate_global_data(&sctx, A)) {
    return 1;
  }

  if (generate_vm_data(&sctx, A)) {
    return 1;
  }

  if (write_addendum(ac[1], A, stage)) {
    return 1;
  }

  return 0;
}
