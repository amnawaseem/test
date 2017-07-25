#include <schism/xml42.h>
#include <schism/gc.h>
#include "generate_config.h"

uint32_t vm_emulate_count(xmlNodePtr guest) {
  uint32_t counter = 0;
  xmlNodePtr vdev;
  uint32_t emulates;

  iterate_over_children_N(guest, "vdev", vdev) //{
    emulates = count_children(vdev, "emulate");
    counter += (emulates ? emulates : 1);
  }

  return counter;
}

uint32_t vm_capability_count(xmlNodePtr guest) {
  return count_children(get_child(guest, "init"), "cap");
}

uint32_t vm_copyins_count(xmlNodePtr guest) {
  return count_children(get_child(guest, "init"), "copy");
}

uint32_t vm_memarea_count(xmlNodePtr guest) {
  return count_children(get_child(guest, "address_space"), "map");
}

xmlNodePtr find_hypervisor_archpage_map(struct scenario_context *sctx, xmlNodePtr guest) {
  const char *archpage_name = get_attribute(get_child(guest, "init"), "arch_page");

  return find_child_by_attribute(query_element(sctx->doc, "/scenario/hypervisor/address_space"),
				"map", "xref", archpage_name);
}

const char *capability_type_string(const char *cap_type) {
  if (strcmp(cap_type, "ipc") == 0) {
    return "CAPABILITY_TYPE_IRQ";
  }

  return NULL;
}

// --------------------------------------------------------------------------
#if 0
#endif

// --------------------------------------------------------------------------

int generate_vm_data(struct scenario_context *sctx, struct addendum *A) {
  struct definition *def_vm, *def_vm_copy, *def_vm_cap;
  struct definition *def_vcpu, *def_sched, *def_mem, *def_em;
  xmlNodePtr guest, guest_as, guest_init, cap, copy;
  xmlNodePtr blob_map, archpage_map;
  xmlNodePtr entry, entry_map;
  xmlNodePtr copy_dst, src_file;
  xmlNodePtr vdev, emulate;
  uint64_t copy_dst_base, copy_src_base, src_file_off, src_file_size;
  const char *gid;
  uint64_t entry_va, archpage_base, guest_ptbl;
  uint32_t *cpumap_list, cpumap_len;
  uint32_t guest_counter, gcpu, hcpu;
  uint32_t map_count, emulate_count, emulate_index;
  struct def_buffer db[2];

  guest_counter = 1;

  iterate_over_children(sctx->doc, "/scenario", "guest", guest) //{
    /* get XML elements */
    guest_as = get_child(guest, "address_space");
    guest_init = get_child(guest, "init");
    entry = get_child(guest, "entry");
    archpage_map = find_hypervisor_archpage_map(sctx, guest);
    blob_map = find_child_by_attribute(query_element(sctx->doc, "/scenario/hypervisor/address_space"),
			"map", "xref", "blob");

    gid = get_attribute(guest, "id");
    entry_map = find_child_by_attribute(guest_as, "map",
			"xref", get_attribute(entry, "bp_xref"));
    cpumap_list = string_to_list(get_attribute(guest, "cpumap"), &cpumap_len);
    map_count = guest_memarea_count(guest);
    emulate_count = vm_emulate_count(guest);

    /* compute derived values */
    if (get_attribute(entry_map, "base") != NULL) {
      entry_va = get_attribute_number(entry_map, "base") +
			get_attribute_number(entry, "bp_offset");
    } else {
      entry_va = 0;
    }

    /* A) create VM structure */
    def_vm = new_addendum_definition(A, "vm", ADDSEC_RO);
    def_buffer_printf(&def_vm->identifier, "vm_%s", gid);
    def_buffer_printf(&def_vm->initializer,
				"{\n"
				"  %d, vm_%s_cpus,\n"
				"  %d, vm_%s_capabilities,\n"
				"  %d, vm_%s_copyins,\n"
				"  0x%lx\n"
				"}",
			cpumap_len, gid,
			vm_capability_count(guest), gid,
			vm_copyins_count(guest), gid,
			entry_va);

    def_vcpu = new_addendum_definition(A, "vm_cpu", ADDSEC_RW);
    def_buffer_printf(&def_vcpu->identifier, "vm_%s_cpus[%d]",
			gid, cpumap_len);
    def_buffer_printf(&def_vcpu->initializer, "{\n");

    /* B) create VM copyins */
    def_vm_copy = new_addendum_definition(A, "vm_copyin", ADDSEC_RO);
    def_buffer_printf(&def_vm_copy->identifier, "vm_%s_copyins[%d]",
			gid, vm_copyins_count(guest));
    def_buffer_printf(&def_vm_copy->initializer, "{\n");
    iterate_over_children_N(guest_init, "copy", copy) //{
      copy_dst = find_child_by_attribute(query_element(sctx->doc, "/scenario/hypervisor/address_space"),
			"map", "xref", get_attribute(copy, "dref"));
      copy_dst_base = get_attribute(copy_dst, "base") ?
			dict_get_hexnumber(copy_dst, "base", cpumap_list[0]) : 0;
      copy_src_base = get_attribute(blob_map, "base") ?
			get_attribute_number(blob_map, "base") : 0;
      src_file = resolve(sctx->doc, get_attribute(copy, "xref"));
      src_file_off = get_attribute(src_file, "offset") ?
			get_attribute_number(src_file, "offset") : 0;
      src_file_size = get_attribute(src_file, "size") ?
			get_attribute_number(src_file, "size") : 0;

      def_buffer_printf(&def_vm_copy->initializer,
                                "  { 0x%lx, 0x%lx, 0x%lx },\n",
			copy_dst_base + get_attribute_number(copy, "offset"),
                        copy_src_base + src_file_off,
			src_file_size);
    }
    def_buffer_printf(&def_vm_copy->initializer, "}");

    /* C) create VM capabilities */
    def_vm_cap = new_addendum_definition(A, "capability", ADDSEC_RO);
    def_buffer_printf(&def_vm_cap->identifier, "vm_%s_capabilities[%d]",
			gid, vm_capability_count(guest));
    def_buffer_printf(&def_vm_cap->initializer, "{\n");
    iterate_over_children_N(guest_init, "cap", cap) //{
      def_buffer_printf(&def_vm_cap->initializer, "  { &vm_%s, %s, 0x%lx },\n",
			get_attribute(cap, "target_xref"),
			capability_type_string(get_attribute(cap, "type")),
			get_attribute_number(cap, "param"));
    }
    def_buffer_printf(&def_vm_cap->initializer, "}");

    /**** per-guest-CPU structures and data ****/

    for (gcpu = 0; gcpu < cpumap_len; gcpu++) {
      hcpu = cpumap_list[gcpu];
      archpage_base = get_attribute(archpage_map, "base") ?
			dict_get_hexnumber(archpage_map, "base", hcpu) : 0;
      guest_ptbl = get_attribute(guest_as, "base") ?
			dict_get_hexnumber(guest_as, "base", gcpu) : 0;

      /* D) create VCPU structure */
      def_buffer_printf(&def_vcpu->initializer,
				"  { &vm_%s, (vm_cpu_state *)0x%lx, %d,\n"
				"    0x%lx, %ld,\n"
				"    %d, vm_%s_cpu%d_memareas, &vm_%s_cpu%d_memtree,\n"
				"    %d, vm_%s_cpu%d_emulates, &vm_%s_cpu%d_emulatetree,\n"
				"    %d, vm_%s_hw_emulates,\n"
				"    %s,\n"
				"    %s,\n"
				"    %s,\n"
				"    1, vm_%s_cpu%d_scheds },\n",
			gid, archpage_base, hcpu,
			guest_ptbl, guest_counter,
			vm_memarea_count(guest), gid, gcpu, gid, gcpu,
			vm_emulate_count(guest), gid, gcpu, gid, gcpu,
			vm_hw_emulate_count(guest), gid,
			vm_master_vdev_name_vtlb(guest, gcpu),
			vm_master_vdev_name_irq(guest, gcpu),
			vm_master_vdev_name_uart(guest, gcpu),
			gid, gcpu);

      /* E) create VCPU sched_entities */
      def_sched = new_addendum_definition(A, "scheduler_entity", ADDSEC_RW);
      def_buffer_printf(&def_sched->identifier, "vm_%s_cpu%d_scheds[%d]",
			gid, gcpu, 1);
      def_buffer_printf(&def_sched->initializer, "{\n"
				"  { vm_%s_cpus + %d, SCHEDULER_CLASS_FAIR_SHARE,"
				" 0, 0, 100, SCHEDULER_STATE_READY, NULL }"
				"\n}",
			gid, gcpu);

      /* F) create VCPU memarea list/tree */
      def_mem = new_addendum_definition(A, "memarea", ADDSEC_RO);
      def_buffer_printf(&def_mem->identifier, "vm_%s_cpu%d_memareas[%d]",
			gid, gcpu, map_count);
      generate_body_memareas(&def_mem->initializer, sctx, A, guest_as, map_count, gcpu,
			query_element(sctx->doc, "/scenario/hypervisor/address_space"));

      init_def_buffer(db+0, 64);
      def_buffer_printf(db+0, "vm_%s_cpu%d_memareas", gid, gcpu);
      init_def_buffer(db+1, 64);
      def_buffer_printf(db+1, "vm_%s_cpu%d_memtree", gid, gcpu);
      generate_memtree(sctx, A, guest_as, map_count, gcpu, db[0].buf, db[1].buf, NULL);

      /* G) create VCPU emulate list/tree */
      def_em = new_addendum_definition(A, "emulate", ADDSEC_RO);
      def_buffer_printf(&def_em->identifier, "vm_%s_cpu%d_emulates[%d]",
			gid, gcpu, emulate_count);
      def_buffer_printf(&def_em->initializer, "{\n");

      iterate_over_children_N(guest, "vdev", vdev) //{
        if (count_children(vdev, "emulate") == 0) {
          def_buffer_printf(&def_em->initializer,
				"  { 0, 0, %s,",
			emulate_type_string(vdev));
          if (is_vdev_global(vdev))
            def_buffer_printf(&def_em->initializer,
				" { &vdev_%s }, 0 },\n",
			get_attribute(vdev, "id"));
          else
            def_buffer_printf(&def_em->initializer,
				" { &vdev_%s_cpu%d }, 0 },\n",
			get_attribute(vdev, "id"), gcpu);
          continue;
        }

        emulate_index = 0;
        iterate_over_children_N(vdev, "emulate", emulate) //{
          def_buffer_printf(&def_em->initializer,
				"  { 0x%lx, 0x%lx, %s,",
			get_attribute_number(emulate, "base"),
			get_attribute_number(emulate, "size"),
			emulate_type_string(vdev));
          if (is_vdev_global(vdev))
            def_buffer_printf(&def_em->initializer,
				" { &vdev_%s }, %ld },\n",
			get_attribute(vdev, "id"), emulate_index);
          else
            def_buffer_printf(&def_em->initializer,
				" { &vdev_%s_cpu%d }, %ld },\n",
			get_attribute(vdev, "id"), gcpu, emulate_index);
          emulate_index++;
        }
      }
      def_buffer_printf(&def_em->initializer, "}");

      init_def_buffer(db+0, 64);
      def_buffer_printf(db+0, "vm_%s_cpu%d_emulates", gid, gcpu);
      init_def_buffer(db+1, 64);
      def_buffer_printf(db+1, "vm_%s_cpu%d_emulatetree", gid, gcpu);
      generate_emulatetree(sctx, A, guest, emulate_count, gcpu, db[0].buf, db[1].buf);
    }

    def_buffer_printf(&def_vcpu->initializer, "}");

    if (generate_vm_vdevs(sctx, A, guest))
      return 1;

    free(cpumap_list);
    guest_counter++;
  }

  return 0;
}
