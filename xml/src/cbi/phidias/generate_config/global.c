#include <sys/time.h>
#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

uint64_t config_pa(struct scenario_context *sctx) {
  xmlNodePtr memreq;

  memreq = resolve(sctx->doc, "config_r");
  if ((memreq == NULL) || (get_attribute(memreq, "base") == NULL))
    return (uint64_t)0;

  return get_attribute_number(memreq, "base");
}

uint64_t config_va(struct scenario_context *sctx) {
  xmlNodePtr map;

  map = find_child_by_attribute(query_element(sctx->doc, "/scenario/hypervisor/address_space"),
				"map", "xref", "config_r");
  if ((map == NULL) || (get_attribute(map, "base") == NULL))
    return (uint64_t)0;

  return get_attribute_number(map, "base");
}

uint64_t ptable_base(struct scenario_context *sctx, int32_t cpu) {
  xmlNodePtr hyp_as;
  const char *dict_str;

  hyp_as = query_element(sctx->doc, "/scenario/hypervisor/address_space");
  dict_str = get_attribute(hyp_as, "base");
  if (dict_str == NULL)
    return (uint64_t)0;

  return dictS_get_hexnumber(dict_str, cpu);
}

uint32_t phys_cpu_count(struct scenario_context *sctx) {
  return query_attribute_number(sctx->doc, "/scenario/hypervisor", "ncpus");
}

uint32_t vm_cpu_count(struct scenario_context *sctx, uint32_t cpu) {
  xmlNodePtr guest;
  uint32_t count = 0;
  uint32_t *cpumap_list, cpumap_len;

  iterate_over_children(sctx->doc, "/scenario", "guest", guest) //{
    cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);
    count += listV_contains(cpumap_list, cpumap_len, cpu) ? 1 : 0;
    free(cpumap_list);
  }

  return count;
}

char *formatted_time() {
  time_t time_now = time(NULL);
  struct tm *time_now_broken = localtime(&time_now);
  char *time_string = malloc(24);

  strftime(time_string, 24, "%Y/%m/%d %H:%M:%S", time_now_broken);
  return time_string;
}

// --------------------------------------------------------------------------

int generate_item_specification(struct scenario_context *sctx, struct addendum *A) {
  xmlNodePtr guest, g_init, irq;
  struct definition *def;

  def = new_addendum_definition(A, "specification", ADDSEC_ROH);
  def_buffer_printf(&def->identifier, "_specification");
  def_buffer_printf(&def->initializer,
				"{\n  \"PHIDSPEC\", 0x%lx, 0x%lx, 0x%lx, %d,"
				" _specification_cpus,"
				" _specification_irq_owners,"
				" &_specification_arch,"
				" \"%s\"\n}",
		config_pa(sctx),
		config_va(sctx),
		ptable_base(sctx, -1),
		phys_cpu_count(sctx),
		formatted_time());

  def = new_addendum_definition(A, "const vm *", ADDSEC_RO);
  def_buffer_printf(&def->identifier, "_specification_irq_owners[IRQID_MAXIMUM]");
  def_buffer_printf(&def->initializer, "{\n");

  iterate_over_children(sctx->doc, "/scenario", "guest", guest) //{
    g_init = get_child(guest, "init");
    iterate_over_children_N(g_init, "irq", irq) //{
      def_buffer_printf(&def->initializer, "  [0x%lx] = &vm_%s,\n",
			get_attribute_number(irq, "vector"),
			get_attribute(guest, "id"));
    }
  }
  def_buffer_printf(&def->initializer, "}");

  return 0;
}

int generate_items_specification_cpu(struct scenario_context *sctx, struct addendum *A) {
  struct definition *def_spec_cpu, *def_memarea, *def_vcpus;
  uint32_t physical_cpu, ncpus;
  xmlNodePtr guest;
  uint32_t *cpumap_list, cpumap_len;
  struct def_buffer db[2];

  ncpus = phys_cpu_count(sctx);

  def_spec_cpu = new_addendum_definition(A, "specification_cpu", ADDSEC_RO);
  def_buffer_printf(&def_spec_cpu->identifier, "_specification_cpus[%d]", ncpus);
  def_buffer_printf(&def_spec_cpu->initializer, "{\n");

  for (physical_cpu = 0; physical_cpu < ncpus; physical_cpu++) {
    uint32_t memarea_count = hypervisor_memarea_count(sctx, physical_cpu);
    uint32_t num_vm_cpu = vm_cpu_count(sctx, physical_cpu);

    /* A) append to _specification_cpus[] array */

    def_buffer_printf(&def_spec_cpu->initializer,
				"  { 0x%lx, %d, cpu%d_memareas, &cpu%d_memtree,",
		ptable_base(sctx, physical_cpu),
		memarea_count,
		physical_cpu,
		physical_cpu);

    if (num_vm_cpu > 0) {
      def_buffer_printf(&def_spec_cpu->initializer,
				" %d, cpu%d_vm_cpus, {} },\n",
		num_vm_cpu,
		physical_cpu);
    } else {
      def_buffer_printf(&def_spec_cpu->initializer,
				" 0, NULL, {} },\n");
    }

    /* B) create cpuX_memareas[] */
    def_memarea = new_addendum_definition(A, "memarea", ADDSEC_RO);
    def_buffer_printf(&def_memarea->identifier, "cpu%d_memareas[%d]",
		physical_cpu,
		memarea_count);
    generate_body_memareas(&def_memarea->initializer, sctx, A,
			query_element(sctx->doc, "/scenario/hypervisor/address_space"),
			memarea_count, physical_cpu, NULL);

    init_def_buffer(db+0, 64);
    def_buffer_printf(db+0, "cpu%d_memareas", physical_cpu);
    init_def_buffer(db+1, 64);
    def_buffer_printf(db+1, "cpu%d_memtree", physical_cpu);

    generate_memtree(sctx, A, query_element(sctx->doc, "/scenario/hypervisor/address_space"),
			memarea_count, physical_cpu,
			db[0].buf, db[1].buf, NULL);

    /* C) create cpuX_vm_cpus[] */
    def_vcpus = new_addendum_definition(A, "vm_cpu *", ADDSEC_RO);
    def_buffer_printf(&def_vcpus->identifier, "cpu%d_vm_cpus[%d]",
			physical_cpu, num_vm_cpu);
    def_buffer_printf(&def_vcpus->initializer, "{\n");
    iterate_over_children(sctx->doc, "/scenario", "guest", guest) //{
      cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);
      if (!listV_contains(cpumap_list, cpumap_len, physical_cpu))
        continue;

      def_buffer_printf(&def_vcpus->initializer,
					"  vm_%s_cpus + %d,\n",
		get_attribute(guest, "id"),
		listV_find_element(cpumap_list, cpumap_len, physical_cpu));
      free(cpumap_list);
    }
    def_buffer_printf(&def_vcpus->initializer, "}");
  }

  def_buffer_printf(&def_spec_cpu->initializer, "}");

  return 0;
}

// --------------------------------------------------------------------------

int generate_global_data(struct scenario_context *sctx, struct addendum *A) {
  struct definition *def;

  if (generate_item_specification(sctx, A)) {
    return 1;
  }

  if (generate_item_specification_arch(sctx, A)) {
    return 1;
  }

  if (generate_items_specification_cpu(sctx, A)) {
    return 1;
  }

  def = new_addendum_definition(A, "uintptr_t", ADDSEC_RW);
  def_buffer_printf(&def->identifier, "__placeholder_rw");
  def_buffer_printf(&def->initializer, "0xbeef");

  def = new_addendum_definition(A, "uintptr_t", ADDSEC_RWS);
  def_buffer_printf(&def->identifier, "__placeholder_rws");
  def_buffer_printf(&def->initializer, "0xf4eebeef");

  return 0;
}
