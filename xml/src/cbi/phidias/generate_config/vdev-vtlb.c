#include <schism/xml42.h>
#include <schism/gc.h>
#include <schism/util.h>
#include "generate_config.h"

struct vtlb_table_entry {
	const char *format;
	uint32_t total_width;
	uint32_t num_levels;
	uint32_t table_size[8];
	uint32_t entry_size_log2;
	const char *format_macro_name;
};

static const struct vtlb_table_entry vtlb_table[] = {
	{ "arm:short", 32, 2, { 14, 10 }, 2, "ARM_SHORT" },
	{ "x86:legacy", 32, 2, { 12, 12 }, 2, "X86_LEGACY" },
	{ "x86:long", 48, 4, { 12, 12, 12, 12 }, 3, "X86_LONG" },
	{ "arm64:g4io40", 40, 4, { 4, 12, 12, 12 }, 3, "ARM64_G4IO40" },
};

const struct vtlb_table_entry *vtlb_format(const char *vtlb_frontend) {
  uint32_t i;

  for (i = 0; i < ARRAYLEN(vtlb_table); i++) {
    if (strcmp(vtlb_frontend, vtlb_table[i].format) == 0)
      return vtlb_table + i;
  }

  return NULL;
}

// --------------------------------------------------------------------------

int generator_vtlb(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def, *def_levelpool;
  uint32_t k, l, m;
  uint32_t is_paravirt = 0;
  const struct vtlb_table_entry *vtlb_fmt = vtlb_format(get_attribute(vdev, "frontend"));
  xmlNodePtr vdev_pv_val;
  uint32_t *cpumap_list, cpumap_len;

  if (vtlb_fmt == NULL) {
    fprintf(stderr, "ERROR: unknown VTLB format.\n");
    return 1;
  }

  if (count_children(vdev, "param") != vtlb_fmt->num_levels) {
    fprintf(stderr, "ERROR: number of VTLB <param> does not match pagetable level depth.\n");
    return 1;
  }

  vdev_pv_val = get_child(vdev, "value");
  if (vdev_pv_val && (strcmp(get_attribute(vdev_pv_val, "type"), "paravirt") == 0)) {
    is_paravirt = get_attribute_number(vdev_pv_val, "value");
  }

  cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);

  for (k = 0; k < cpumap_len; k++) {
    xmlNodePtr level_backing;

    /* VTLB */
    def = new_addendum_definition(A, "emulate_vtlb", ADDSEC_RW);
    def_buffer_printf(&def->identifier, "vdev_%s_cpu%d",
			get_attribute(vdev, "id"), k);
    def_buffer_printf(&def->initializer,
				"{\n  VTLB_PAGING_FORMAT_%s, %d,\n"
				"  8, vdev_%s_cpu%d_instances,\n"
				"  %d, vdev_%s_cpu%d_levelpools, VTLB_NO_ACTIVE_INSTANCE\n}",
			vtlb_fmt->format_macro_name, is_paravirt,
			get_attribute(vdev, "id"), k,
			vtlb_fmt->num_levels,
			get_attribute(vdev, "id"), k);

    /* VTLB instances (slots) */
    def = new_addendum_definition(A, "emulate_vtlb_instance", ADDSEC_RW);
    def_buffer_printf(&def->identifier, "vdev_%s_cpu%d_instances[%d]",
			get_attribute(vdev, "id"), k, 8);
    def_buffer_printf(&def->initializer, "{}");

    /* VTLB backing pools */
    def = new_addendum_definition(A, "emulate_vtlb_pool", ADDSEC_RO);
    def_buffer_printf(&def->identifier, "vdev_%s_cpu%d_levelpools[%d]",
			get_attribute(vdev, "id"), k, vtlb_fmt->num_levels);
    def_buffer_printf(&def->initializer, "{\n");

    l = 0;
    iterate_over_children_N(vdev, "param", level_backing) {
      uint32_t physical_cpu = cpumap_list[k];
      xmlNodePtr backing_map = find_hypervisor_map(sctx, get_attribute(level_backing, "xref"));
      xmlNodePtr backing_memreq = resolve(sctx->doc, get_attribute(level_backing, "xref"));
      uint32_t shift_accumulated = vtlb_fmt->total_width;
      uint32_t backing_entries = get_attribute_number(backing_memreq, "size") >> vtlb_fmt->table_size[l];
      uint64_t map_base, memreq_base;

      map_base = (get_attribute(backing_map, "base") != NULL) ?
			dict_get_hexnumber(backing_map, "base", physical_cpu) : 0;
      memreq_base = (get_attribute(backing_memreq, "base") != NULL) ?
			dict_get_hexnumber(backing_memreq, "base", physical_cpu) : 0;

      for (m = 0; m <= l; m++) {
        shift_accumulated -= vtlb_fmt->table_size[m] - vtlb_fmt->entry_size_log2;
      }

      def_buffer_printf(&def->initializer,
				"  { %d, %d, 0x%lx, vdev_%s_cpu%d_levelpool%d_bitmap,\n"
				"    %d, 0x%lx, 0x%lx },\n",
			vtlb_fmt->table_size[l],
			vtlb_fmt->entry_size_log2,
			backing_entries,
			get_attribute(vdev, "id"), k, l,
			shift_accumulated, map_base, memreq_base);

      def_levelpool = new_addendum_definition(A, "uint32_t", ADDSEC_RW);
      def_buffer_printf(&def_levelpool->identifier, "vdev_%s_cpu%d_levelpool%d_bitmap[%d]",
			get_attribute(vdev, "id"), k, l, (backing_entries + 31) >> 5);
      def_buffer_printf(&def_levelpool->initializer, "{}");

      l++;
    }}

    def_buffer_printf(&def->initializer, "}");
  }

  free(cpumap_list);

  return 0;
}
