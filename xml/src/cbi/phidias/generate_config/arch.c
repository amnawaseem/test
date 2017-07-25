#include <sys/time.h>
#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

// ---------------------------------------------------------------------------

static uint32_t _gisa_x86_gdt_32[] = {
	0x00000000, 0x00000000,
	0x0000ffff, 0x00cf9b00,
	0x0000ffff, 0x00cf9300,
	0x00000067, 0x00008900,
	0x00000000, 0x00000000,
	0x0000ffff, 0x00cffb00,
	0x0000ffff, 0x00cff300,
};

static uint32_t _gisa_x86_gdt_64[] = {
	0x00000000, 0x00000000,
	0x0000ffff, 0x00a09b00,
	0x0000ffff, 0x00a09300,
	0x00000067, 0x00008900,
	0x00000000, 0x00000000,
	0x0000ffff, 0x00a0fb00,
	0x0000ffff, 0x00a0f300,
};

static int _gis_arch_x86(struct scenario_context *sctx, struct addendum *A) {
  xmlNodePtr hyp_feature;
  const char *arch_width;
  struct definition *def;
  uint32_t *gdtdata = NULL;
  uint64_t symbol_location_tss;
  uint32_t i;

  hyp_feature = find_child_by_attribute(query_element(sctx->doc, "/scenario/hypervisor"),
					"feature", "name", "arch_width");
  arch_width = get_attribute(hyp_feature, "value");

  if (strcmp(arch_width, "64") == 0) {
    gdtdata = _gisa_x86_gdt_64;
  } else if (strcmp(arch_width, "32") == 0) {
    gdtdata = _gisa_x86_gdt_32;
  } else
    abort();

  def = new_addendum_definition(A, "specification_arch", ADDSEC_RO);
  def_buffer_printf(&def->identifier, "_specification_arch");
  def_buffer_printf(&def->initializer, "{\n  NULL, _archspec_gdt, _archspec_tss\n}");

  def = new_addendum_definition(A, "uint32_t", ADDSEC_RW);
  def_buffer_printf(&def->identifier, "_archspec_gdt[14]");
  def_buffer_printf(&def->initializer, "{\n");

  symbol_location_tss = query_symbol(A, "_archspec_tss");
  gdtdata[6] |= (symbol_location_tss << 16);
  gdtdata[7] |= (symbol_location_tss >> 16) & 0xff;
  gdtdata[7] |= (symbol_location_tss & 0xff000000);
  gdtdata[8] = (symbol_location_tss >> 32);

  for (i = 0; i < 7; i++) {
    def_buffer_printf(&def->initializer,
		"  0x%08x, 0x%08x,\n", gdtdata[2*i], gdtdata[2*i+1]);
  }
  def_buffer_printf(&def->initializer, "}");

  def = new_addendum_definition(A, "uint32_t", ADDSEC_RW);
  def_buffer_printf(&def->identifier, "_archspec_tss[104 >> 2]");
  // def_buffer_printf(&def->initializer, "{ 0, 0, 0x10, }");
  def_buffer_printf(&def->initializer, "{ 0, 0, 0xf40a4ff0, 0xffffffff, }");

  return 0;
}

// ---------------------------------------------------------------------------

int generate_item_specification_arch(struct scenario_context *sctx, struct addendum *A) {
  const char *arch = query_attribute(sctx->doc, "/scenario/platform/arch", "id");
  struct definition *def;

  if (strcmp(arch, "x86") == 0) {
    return _gis_arch_x86(sctx, A);
  }

  /* default empty arch spec */
  def = new_addendum_definition(A, "specification_arch", ADDSEC_RO);
  def_buffer_printf(&def->identifier, "_specification_arch");
  def_buffer_printf(&def->initializer, "{}");

  return 0;
}

// ---------------------------------------------------------------------------
