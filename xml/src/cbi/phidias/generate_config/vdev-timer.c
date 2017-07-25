#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

int generator_timer_sp804(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;

  (void)sctx; (void)guest;

  def = new_addendum_definition(A, "emulate_timer_sp804", ADDSEC_RWS);
  def_buffer_printf(&def->identifier, "vdev_%s",
                        get_attribute(vdev, "id"));
  def_buffer_printf(&def->initializer, "{}");

  return 0;
}

int generator_timer_mpcore(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;
  uint32_t k;
  uint32_t *cpumap_list, cpumap_len;

  (void)sctx;

  cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);
  free(cpumap_list);

  for (k = 0; k < cpumap_len; k++) {
    def = new_addendum_definition(A, "emulate_timer_mpcore", ADDSEC_RW);
    def_buffer_printf(&def->identifier, "vdev_%s_cpu%d",
                        get_attribute(vdev, "id"), k);
    def_buffer_printf(&def->initializer, "{}");
  }

  return 0;
}

int generator_timer_armcp14(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;
  uint32_t k;
  uint32_t *cpumap_list, cpumap_len;

  (void)sctx;

  cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);
  free(cpumap_list);

  for (k = 0; k < cpumap_len; k++) {
    def = new_addendum_definition(A, "emulate_timer_armcp14", ADDSEC_RW);
    def_buffer_printf(&def->identifier, "vdev_%s_cpu%d",
                        get_attribute(vdev, "id"), k);
    def_buffer_printf(&def->initializer, "{}");
  }

  return 0;
}
