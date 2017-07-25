#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

static int build_memory_array(xmlNodePtr vdev, const char *param, const char *def_param, uint64_t *defval) {
  xmlNodePtr value;

  *defval = 0UL;

  iterate_over_children_N(vdev, "value", value) //{
    if (strcmp(get_attribute(value, "type"), def_param) == 0) {
      *defval = get_attribute_number(value, "value");
      break;
    }
  }

  iterate_over_children_N(vdev, "value", value) //{
    if (strcmp(get_attribute(value, "type"), param) == 0) {
      if (get_attribute_number(value, "value") != *defval) {
        return 1;
      }
    }
  }

  return 0;
}

// --------------------------------------------------------------------------

int generator_memory32(struct scenario_context *sctx, struct addendum *A,
			xmlNodePtr guest, xmlNodePtr vdev) {
  struct definition *def;
  int hasarray_value, hasarray_mem_rmask, hasarray_mem_wmask, hasarray_hw_rmask, hasarray_hw_wmask;
  uint64_t def_value, def_mem_rmask, def_mem_wmask, def_hw_rmask, def_hw_wmask;
  uint64_t hw_address = 0UL;
  xmlNodePtr param, value;
  struct def_buffer mem_flags;

  (void)guest;

  init_def_buffer(&mem_flags, 256);

  hasarray_value = build_memory_array(vdev, "mem_value", "default_mem_value", &def_value);
  if (!hasarray_value) def_buffer_printf(&mem_flags, "|EMULATE_MEMORY_FLAG_SINGLE_VALUE");

  hasarray_mem_rmask = build_memory_array(vdev, "mask_mem_r", "default_mask_mem_r", &def_mem_rmask);
  if (!hasarray_mem_rmask) def_buffer_printf(&mem_flags, "|EMULATE_MEMORY_FLAG_SINGLE_MEM_RMASK");

  hasarray_mem_wmask = build_memory_array(vdev, "mask_mem_w", "default_mask_mem_w", &def_mem_wmask);
  if (!hasarray_mem_wmask) def_buffer_printf(&mem_flags, "|EMULATE_MEMORY_FLAG_SINGLE_MEM_WMASK");

  hasarray_hw_rmask = build_memory_array(vdev, "mask_hw_r", "default_mask_hw_r", &def_hw_rmask);
  if (!hasarray_hw_rmask) def_buffer_printf(&mem_flags, "|EMULATE_MEMORY_FLAG_SINGLE_HW_RMASK");

  hasarray_hw_wmask = build_memory_array(vdev, "mask_hw_w", "default_mask_hw_w", &def_hw_wmask);
  if (!hasarray_hw_wmask) def_buffer_printf(&mem_flags, "|EMULATE_MEMORY_FLAG_SINGLE_HW_WMASK");


  def = new_addendum_definition(A, "emulate_memory", ADDSEC_RO);
  def_buffer_printf(&def->identifier, "vdev_%s",
			get_attribute(vdev, "id"));
  def_buffer_printf(&def->initializer,
				"{\n"
				"  %s,\n"
				"  0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx,\n",
			mem_flags.buf + 1,
			def_value, def_mem_rmask, def_mem_wmask, def_hw_rmask, def_hw_wmask);

  if (hasarray_value)
    def_buffer_printf(&def->initializer, "  vdev_%s_values,\n",
			get_attribute(vdev, "id"));
  else
    def_buffer_printf(&def->initializer, "  NULL,\n");

  iterate_over_children_N(vdev, "param", param) //{
    if (strcmp(get_attribute(param, "type"), "hardware") == 0) {
      xmlNodePtr hw_map = find_child_by_attribute(query_element(sctx->doc, "/scenario/hypervisor/address_space"),
				"map", "xref", get_attribute(param, "xref"));
      // xmlNodePtr hw_memreq = find_memreq(scene, hw_map->attrs[MAP_ATTR_XREF].value.string);
      // xmlNodePtr hw_device = find_device(scene, hw_map->attrs[MAP_ATTR_XREF].value.string);

      if (get_attribute(hw_map, "base") != NULL)
        hw_address += dict_get_hexnumber(hw_map, "base", -1);
      else
        hw_address = 0;
    }
  }
  iterate_over_children_N(vdev, "value", value) //{
    if (strcmp(get_attribute(value, "type"), "hardware_offset") == 0) {
      hw_address += get_attribute_number(value, "value");
      break;
    }
  }
  def_buffer_printf(&def->initializer, "  (volatile uint32_t *)0x%lx,\n",
			hw_address);

  if (hasarray_mem_rmask)
    def_buffer_printf(&def->initializer, "  vdev_%s_mem_rmask,\n",
			get_attribute(vdev, "id"));
  else
    def_buffer_printf(&def->initializer, "  NULL,\n");

  if (hasarray_mem_wmask)
    def_buffer_printf(&def->initializer, "  vdev_%s_mem_wmask,\n",
			get_attribute(vdev, "id"));
  else
    def_buffer_printf(&def->initializer, "  NULL,\n");

  if (hasarray_hw_rmask)
    def_buffer_printf(&def->initializer, "  vdev_%s_hw_rmask,\n",
			get_attribute(vdev, "id"));
  else
    def_buffer_printf(&def->initializer, "  NULL,\n");

  if (hasarray_hw_wmask)
    def_buffer_printf(&def->initializer, "  vdev_%s_hw_wmask\n}",
			get_attribute(vdev, "id"));
  else
    def_buffer_printf(&def->initializer, "  NULL\n}");

  return 0;
}
