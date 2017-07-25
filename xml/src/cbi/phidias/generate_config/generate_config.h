#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define	ADDSEC_ROH		0
#define	ADDSEC_RO		1
#define	ADDSEC_RW		2
#define	ADDSEC_RWS		3

#define	COMMON_ARGS		struct scenario_context *sctx, struct addendum *A

// arch.c

extern int generate_item_specification_arch(COMMON_ARGS);

// global.c

extern int generate_item_specification(COMMON_ARGS);
extern int generate_items_specification_cpu(COMMON_ARGS);

extern int generate_global_data(COMMON_ARGS);

// mmu.c

extern uint32_t hypervisor_memarea_count(struct scenario_context *sctx, uint32_t physical_cpu);
extern uint32_t hypervisor_memarea_index(xmlNodePtr map);
extern uint32_t guest_memarea_count(xmlNodePtr guest);

extern void generate_body_memareas(struct def_buffer *dbf, COMMON_ARGS,
			xmlNodePtr mmu, uint32_t memarea_count, uint32_t cpu,
			xmlNodePtr reference_mmu);

// trees.c

extern void generate_memtree(COMMON_ARGS, xmlNodePtr address_space,
			uint32_t memarea_count, uint32_t cpu,
			const char *memarea_name,
			const char *top_item_name, const char *top_item_xref);
extern void generate_memtree_item(struct addendum *A,
			xmlNodePtr *sortlist, uint32_t *sort_interval,
			uint32_t split_item,
			const char *memarea_name, const char *name);

extern void generate_emulatetree(COMMON_ARGS, xmlNodePtr guest,
			uint32_t emulate_count, uint32_t cpu,
			const char *emulate_name,
			const char *top_item_name);
extern void generate_emulatetree_item(struct addendum *A,
			xmlNodePtr *sortlist, uint32_t *indices,
			uint32_t *sort_interval, uint32_t split_item,
			const char *emulate_name, const char *name);

// vm.c

extern int generate_vm_data(COMMON_ARGS);

#if 0
struct percpu_data_t {
  uint32_t num_vcpus;
};

struct nametable_entry {
  const char *arch_match;
  const char *board_match;
  const char *name;
  unsigned int index;
};

#define	NONCOREMAPS_BASE	32
#endif
// main.c

uint64_t base_address;

// vdevs.c

typedef int (vdev_generator)(COMMON_ARGS, xmlNodePtr guest, xmlNodePtr vdev);

extern const struct vdev_table_entry *walk_vdev_table(xmlNodePtr vdev);
extern const char *emulate_type_string(xmlNodePtr vdev);
extern int is_vdev_global(xmlNodePtr vdev);

extern char *vm_master_vdev_name(xmlNodePtr guest, uint32_t cpu, const char *category);

static inline char *vm_master_vdev_name_vtlb(xmlNodePtr guest, uint32_t cpu) {
	return vm_master_vdev_name(guest, cpu, "vtlb");
}
static inline char *vm_master_vdev_name_irq(xmlNodePtr guest, uint32_t cpu) {
	return vm_master_vdev_name(guest, cpu, "irq_controller");
}
static inline char *vm_master_vdev_name_uart(xmlNodePtr guest, uint32_t cpu) {
	return vm_master_vdev_name(guest, cpu, "serial");
}

extern uint32_t vm_hw_emulate_count(xmlNodePtr guest);
extern int generate_vm_vdevs(COMMON_ARGS, xmlNodePtr guest);

// vdev-vtlb.c

extern vdev_generator generator_vtlb;

// vdev-memory.c

extern vdev_generator generator_memory32;

// vdev-irq.c

extern vdev_generator generator_irq_gic;
extern vdev_generator generator_irq_lapic;

// vdev-uart.c

extern vdev_generator generator_uart_pl011;
extern vdev_generator generator_uart_16550;
extern vdev_generator generator_uart_pic32;

// vdev-timer.c

extern vdev_generator generator_timer_sp804;
extern vdev_generator generator_timer_mpcore;
extern vdev_generator generator_timer_armcp14;

// vdev-clock.c

extern vdev_generator generator_clock_mpcore;

// symbols.c

extern uint64_t query_symbol(struct addendum *A, const char *symbol_name);
extern void load_symbol_locations(const char *fn);
