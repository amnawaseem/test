#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

int generator_clock_mpcore(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;

  (void)sctx; (void)guest;

  def = new_addendum_definition(A, "emulate_clock_mpcore", ADDSEC_RWS);
  def_buffer_printf(&def->identifier, "vdev_%s",
                        get_attribute(vdev, "id"));
  def_buffer_printf(&def->initializer, "{}");

  return 0;
}
