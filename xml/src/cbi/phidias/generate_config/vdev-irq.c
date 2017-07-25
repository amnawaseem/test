#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

int generator_irq_gic(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;
  uint32_t j;
  uint32_t *cpumap_list, cpumap_len;

  (void)sctx;

  cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);
  free(cpumap_list);

  for (j = 0; j < cpumap_len; j++) {
    def = new_addendum_definition(A, "emulate_irq_gic", ADDSEC_RW);
    def_buffer_printf(&def->identifier, "vdev_%s_cpu%d",
                        get_attribute(vdev, "id"), j);
    def_buffer_printf(&def->initializer,
			"{ 0, 0, 0, 0, {}, {}, {}, {}, &vdev_%s_dist, {}, 0 }",
			get_attribute(vdev, "id"));
  }

  def = new_addendum_definition(A, "emulate_irq_gic_distributor", ADDSEC_RWS);
  def_buffer_printf(&def->identifier, "vdev_%s_dist",
			get_attribute(vdev, "id"));
  def_buffer_printf(&def->initializer, "{}");

  return 0;
}

int generator_irq_lapic(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;
  uint32_t j;
  uint32_t *cpumap_list, cpumap_len;

  (void)sctx;

  cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);
  free(cpumap_list);

  for (j = 0; j < cpumap_len; j++) {
    def = new_addendum_definition(A, "emulate_irq_lapic", ADDSEC_RW);
    def_buffer_printf(&def->identifier, "vdev_%s_cpu%d",
                        get_attribute(vdev, "id"), j);
    def_buffer_printf(&def->initializer, "{}");
  }

  return 0;
}
