#include <schism/xml42.h>
#include <schism/gc.h>
#include <schism/util.h>
#include "generate_config.h"

struct vdev_table_entry {
	const char *type;
	const char *frontend;
	const char *phidias_type;
	int is_global;
	uint32_t expected_bars;
	vdev_generator *generator;
	int is_hardware;
};

static const struct vdev_table_entry vdev_table[] = {
	{ "clock", "mpcore", "EMULATE_TYPE_CLOCK_MPCORE", 1, 1, &generator_clock_mpcore, 0 },
	{ "irq_controller", "lapic", "EMULATE_TYPE_IRQ_LAPIC", 0, 2, &generator_irq_lapic, 0 },
	{ "irq_controller", "arm_gic", "EMULATE_TYPE_IRQ_GIC", 0, 2, &generator_irq_gic, 0 },
	{ "irq_controller", "arm_gic_virtext", "EMULATE_TYPE_IRQ_GIC_VIRTEXT", 0, 1, &generator_irq_gic, 1 },
	{ "memory32", NULL, "EMULATE_TYPE_MEMORY_32BIT", 1, 1, &generator_memory32, 0 },
	{ "serial", "16550", "EMULATE_TYPE_UART_16550", 1, 1, &generator_uart_16550, 0 },
	{ "serial", "pl011", "EMULATE_TYPE_UART_PL011", 1, 1, &generator_uart_pl011, 0 },
	{ "serial", "pic32", "EMULATE_TYPE_UART_PIC32", 1, 1, &generator_uart_pic32, 0 },
	{ "timer", "sp804", "EMULATE_TYPE_TIMER_SP804", 1, 1, &generator_timer_sp804, 0 },
	{ "timer", "mpcore", "EMULATE_TYPE_TIMER_MPCORE", 0, 1, &generator_timer_mpcore, 0 },
	{ "timer", "armcp14", "EMULATE_TYPE_TIMER_ARMCP14", 0, 0, &generator_timer_armcp14, 1 },
	{ "vtlb", NULL, "EMULATE_TYPE_VTLB", 0, 0, &generator_vtlb, 0 },
};

const struct vdev_table_entry *walk_vdev_table(xmlNodePtr vdev) {
  const char *vdevtype, *vdevfrontend;
  uint32_t i;

  vdevtype = get_attribute(vdev, "type");
  vdevfrontend = get_attribute(vdev, "frontend");

  for (i = 0; i < ARRAYLEN(vdev_table); i++) {
    if (strcmp(vdevtype, vdev_table[i].type) != 0)
      continue;

    if (vdev_table[i].frontend == NULL)
      return vdev_table + i;

    if (strcmp(vdevfrontend, vdev_table[i].frontend) == 0)
      return vdev_table + i;
  }

  return NULL;
}

const char *emulate_type_string(xmlNodePtr vdev) {
  const struct vdev_table_entry *entry = walk_vdev_table(vdev);

  if (entry)
    return entry->phidias_type;

  return "UNKNOWN_TYPE";
}

int is_vdev_global(xmlNodePtr vdev) {
  const struct vdev_table_entry *entry = walk_vdev_table(vdev);

  if (entry)
    return entry->is_global;

  return 0;
}

char *vm_master_vdev_name(xmlNodePtr guest, uint32_t cpu, const char *category) {
  xmlNodePtr vdev;
  char *name;
  uint32_t counter = 0;
  int len;

  iterate_over_children_N(guest, "vdev", vdev) //{
    uint32_t emulates = count_children(vdev, "emulate");
    
    if ((strcmp(get_attribute(vdev, "type"), category) == 0) &&
	(get_attribute(vdev, "master") != NULL)) {
      name = "";
      len = snprintf(name, 0, "vm_%s_cpu%d_emulates + %d",
			get_attribute(guest, "id"), cpu, counter);
      name = malloc(len+2);
      snprintf(name, len+1, "vm_%s_cpu%d_emulates + %d",
			get_attribute(guest, "id"), cpu, counter);
      return name;
    }

    counter += (emulates ? emulates : 1);
  }

  return "NULL";
}

// --------------------------------------------------------------------------

uint32_t vm_hw_emulate_count(xmlNodePtr guest) {
  uint32_t counter = 0;
  xmlNodePtr vdev;
  const struct vdev_table_entry *vte;

  iterate_over_children_N(guest, "vdev", vdev) //{
    vte = walk_vdev_table(vdev);
    if (vte && vte->is_hardware)
      counter++;
  }

  return counter;
}

// --------------------------------------------------------------------------

int generate_vm_vdevs(struct scenario_context *sctx, struct addendum *A, xmlNodePtr guest) {
  xmlNodePtr vdev;
  const char *vdevid;
  uint64_t j;
  uint32_t vdevbarcount, hwvdevcount;
  uint32_t *hwindices;
  struct definition *def;
  const struct vdev_table_entry *entry;

  hwindices = malloc(sizeof(uint32_t) * vm_hw_emulate_count(guest));
  hwvdevcount = 0;
  vdevbarcount = 0;

  iterate_over_children_N(guest, "vdev", vdev) //{
    entry = walk_vdev_table(vdev);
    vdevid = get_attribute(vdev, "id");

    if (!entry) {
      fprintf(stderr, "ERROR: no vdev entry for %s.\n", vdevid);
      return 1;
    }
    if (!entry->generator) {
      fprintf(stderr, "ERROR: no vdev generator for %s.\n", vdevid);
      return 1;
    }
    if (entry->expected_bars != count_children(vdev, "emulate")) {
      fprintf(stderr, "ERROR: number of BARs doesn't match expectation for %s (%d != %d).\n",
			vdevid,
			count_children(vdev, "emulate"), entry->expected_bars);
      return 1;
    }

    if (entry->generator(sctx, A, guest, vdev))
        return 1;

    if (entry->is_hardware) {
      hwindices[hwvdevcount] = vdevbarcount;
      hwvdevcount++;
    }

    vdevbarcount += count_children(vdev, "emulate") ?: 1;
  }

  def = new_addendum_definition(A, "const emulate *", ADDSEC_RO);
  def_buffer_printf(&def->identifier, "vm_%s_hw_emulates[%d]",
			get_attribute(guest, "id"), hwvdevcount);
  def_buffer_printf(&def->initializer, "{");
  for (j = 0; j < hwvdevcount; j++) {
    def_buffer_printf(&def->initializer, " vm_%s_cpu0_emulates + %d,",
			get_attribute(guest, "id"),
			hwindices[j]);
  }
  def_buffer_printf(&def->initializer, " }");

  return 0;
}
