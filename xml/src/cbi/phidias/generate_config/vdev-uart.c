#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

int generator_uart_pl011(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;

  (void)sctx; (void)guest;

  def = new_addendum_definition(A, "emulate_uart_pl011", ADDSEC_RWS);
  def_buffer_printf(&def->identifier, "vdev_%s",
			get_attribute(vdev, "id"));
  def_buffer_printf(&def->initializer, "{}");

  return 0;
}

int generator_uart_16550(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;

  (void)sctx; (void)guest;

  def = new_addendum_definition(A, "emulate_uart_16550", ADDSEC_RWS);
  def_buffer_printf(&def->identifier, "vdev_%s",
			get_attribute(vdev, "id"));
  def_buffer_printf(&def->initializer, "{}");

  return 0;
}

int generator_uart_pic32(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;

  (void)sctx; (void)guest;

  def = new_addendum_definition(A, "emulate_uart_pic32", ADDSEC_RWS);
  def_buffer_printf(&def->identifier, "vdev_%s",
			get_attribute(vdev, "id"));
  def_buffer_printf(&def->initializer, "{}");

  return 0;
}
