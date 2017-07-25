#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>
#include <drivers/clock.h>
#include <arch/cpu_ops.h>

static void emulate_vtlb_initialize_slot(emulate_vtlb *vtlb, uint32_t i);
static void emulate_vtlb_activate_slot(const vm_cpu *vm_cpu_switch, emulate_vtlb *vtlb, uint32_t i);

static uintptr_t emulate_vtlb_allocate_directory(emulate_vtlb *vtlb, uint32_t level);
static void emulate_vtlb_return_directory(emulate_vtlb *vtlb, uint32_t level, uintptr_t directory);

static void emulate_vtlb_map_range_slot(emulate_vtlb *vtlb, uint32_t i,
			uintptr_t vaddr, uintptr_t paddr, uintptr_t size, uint32_t flags);
static inline void emulate_vtlb_map_area_slot(emulate_vtlb *vtlb, uint32_t i, const struct memarea_ *area);
static void emulate_vtlb_map_level_page_slot(emulate_vtlb *vtlb, uint32_t i, uint32_t level,
			uintptr_t vaddr, uintptr_t paddr, uint32_t flags);

static void emulate_vtlb_unmap_slot_level_address(emulate_vtlb *vtlb, uint32_t i, uint32_t level,
				uintptr_t base, uintptr_t address);

//

#ifdef FEATURE_VTLB_ASSERTIONS
void emulate_vtlb_assert_bounds(emulate_vtlb *vtlb, uint32_t level, uintptr_t address) {
	uintptr_t min = vtlb->level_pools[level].virtual_address;
	uintptr_t max = min + (vtlb->level_pools[level].count << vtlb->level_pools[level].dirsize_log2);

	if ((address < min) || (address >= max)) {
		printf("BOUNDS PANIC: %x NE [%x, %x] C %x\r\n", address, min, max, __builtin_return_address(0));
		panic();
	}
}
#endif

void emulate_vtlb_initialize(const vm_cpu *vm_cpu_reset, emulate_vtlb *vtlb) {
	uint32_t i;

	vtlb->active_instance = VTLB_NO_ACTIVE_INSTANCE;

	for (i = 0; i < vtlb->num_levels; i++) {
		memset((void *)vtlb->level_pools[i].virtual_address,
			0,
			vtlb->level_pools[i].count << vtlb->level_pools[i].dirsize_log2);

		memset(vtlb->level_pools[i].bitmap,
			0,
			((vtlb->level_pools[i].count + 31) >> 5) << 2);
	}

	for (i = 0; i < vtlb->num_instances; i++) {
		vtlb->instances[i].identifier = 2+i;
		emulate_vtlb_initialize_slot(vtlb, i);
		// printf("Slot: %x\r\n", vtlb->instances[i].pagetable_address);
	}

	emulate_vtlb_activate_slot(vm_cpu_reset, vtlb, 0);
}

static void emulate_vtlb_initialize_slot(emulate_vtlb *vtlb, uint32_t i) {
	uint32_t j;

	vtlb->instances[i].pagetable_address = emulate_vtlb_allocate_directory(vtlb, 0);
	if (vtlb->instances[i].pagetable_address == VTLB_ALLOCATOR_EMPTY)
		panic();

	vtlb->instances[i].vm_pagetable_address = VTLB_INVALID_PAGETABLE;
	vtlb->instances[i].vm_identifier = VTLB_INVALID_IDENTIFIER;

	if (vtlb->is_paravirt == 0)
		return;

	for (j = 0; j < _specification.cpus[cpu_number].num_memareas; j++) {
		const memarea *area = _specification.cpus[cpu_number].memareas + j;

		if (!(area->flags & MEMAREA_FLAG_G))
			continue;

		emulate_vtlb_map_area_slot(vtlb, i, area);
	}
}

static void emulate_vtlb_activate_slot(const vm_cpu *vm_cpu_switch, emulate_vtlb *vtlb, uint32_t i) {
	vtlb->instances[i].last_usage = clock_read();

	vtlb->active_instance = i;
	emulate_vtlb_ensure_guest_paging(vm_cpu_switch);
/*
	printf("New active instance: [%x] %d, (%x:%x)\r\n", vtlb, vtlb->active_instance,
		vm_cpu_switch->pagetable_base, vm_cpu_switch->pagetable_identifier);
*/
}

// POOL BITMAP MANAGEMENT

static uintptr_t emulate_vtlb_allocate_directory(emulate_vtlb *vtlb, uint32_t level) {
	uint32_t i, j;
	const emulate_vtlb_pool *pool = vtlb->level_pools + level;

	for (i = 0; i < (pool->count+31) >> 5; i++) {
		if (pool->bitmap[i] == ~0U) {
			continue;
		}
		for (j = 0; j < 32; j++) {
			if (!(pool->bitmap[i] & (1U << j))) {
				pool->bitmap[i] |= (1U << j);
				memset((void *)pool->virtual_address + (((i<<5)+j) << pool->dirsize_log2),
					0, 1 << pool->dirsize_log2);
				return pool->physical_address + (((i<<5)+j) << pool->dirsize_log2);
			}
		}
	}

	return VTLB_ALLOCATOR_EMPTY;
}

static void emulate_vtlb_return_directory(emulate_vtlb *vtlb, uint32_t level, uintptr_t directory) {
	const emulate_vtlb_pool *pool = vtlb->level_pools + level;
	uint32_t index = (directory - pool->physical_address) >> pool->dirsize_log2;

	// assert that the bit was set until now...

	pool->bitmap[index >> 5] &= ~(1U << (index & 31));
}

static void emulate_vtlb_evict_on_level(emulate_vtlb *vtlb, uint32_t level) {

}

static inline uintptr_t emulate_vtlb_pool_vaddr(emulate_vtlb *vtlb, uint32_t level, uintptr_t pool_paddr) {
	return pool_paddr - vtlb->level_pools[level].physical_address + vtlb->level_pools[level].virtual_address;
}

// ADD MAPPINGS

void emulate_vtlb_map_range(const vm_cpu *vm_cpu_map,
			uintptr_t vaddr, uintptr_t paddr, uintptr_t size, uint32_t flags) {
	emulate_vtlb *vtlb = vm_cpu_map->vtlb_emulate->control.vtlb;

	emulate_vtlb_map_range_slot(vtlb, vtlb->active_instance, vaddr, paddr, size, flags);
}

static void emulate_vtlb_map_range_slot(emulate_vtlb *vtlb, uint32_t i,
			uintptr_t vaddr, uintptr_t paddr, uintptr_t size, uint32_t flags) {
	uint32_t level;
	uintptr_t mask;

	while (size) {
		for (level = 0; level < vtlb->num_levels; level++) {
			mask = (1UL << vtlb->level_pools[level].shift) - 1;
			if (((vaddr | paddr | size) & mask) == 0) {
				break;
			}

			if (level == vtlb->num_levels) {
				printf("Panic: VTLB asked to map sub-page range.\r\n");
				panic();
			}

			emulate_vtlb_map_level_page_slot(vtlb, i, level, vaddr, paddr, flags);

			vaddr += mask + 1;
			paddr += mask + 1;
			size -= mask + 1;
		}
	}
}

static inline void emulate_vtlb_map_area_slot(emulate_vtlb *vtlb, uint32_t i, const struct memarea_ *area) {
	emulate_vtlb_map_range_slot(vtlb, i, area->vaddr, area->paddr, area->size, area->flags);
}

static void emulate_vtlb_map_level_page_slot(emulate_vtlb *vtlb, uint32_t i, uint32_t level,
			uintptr_t vaddr, uintptr_t paddr, uint32_t flags) {
	uint32_t j;
	emulate_vtlb_instance *instance = vtlb->instances + i;
	uintptr_t walk_address;
	uintptr_t entry_address;

	if (instance->pagetable_address == VTLB_INVALID_PAGETABLE) {
		instance->pagetable_address = emulate_vtlb_allocate_directory(vtlb, 0);
		if (instance->pagetable_address == VTLB_ALLOCATOR_EMPTY)
			panic();
	}

	walk_address = instance->pagetable_address;

	if (walk_address == 0) {
		printf("Instance PTAddr invalid\r\n");
		panic();
	}

	// printf("Map %x->%x (F%x) L%d (PA %x)\r\n", (uint32_t)vaddr, (uint32_t)paddr, flags, level, (uint32_t)walk_address);

	for (j = 0; j <= level; j++) {
		uint32_t vaddr_bits;

		vaddr_bits = vaddr >> vtlb->level_pools[j].shift;
		vaddr_bits &= (1U << (vtlb->level_pools[j].dirsize_log2 - vtlb->level_pools[j].entrysize_log2)) - 1;
		entry_address = emulate_vtlb_pool_vaddr(vtlb, j, walk_address);
		entry_address += (vaddr_bits << vtlb->level_pools[j].entrysize_log2);

		if (j == level)
			break;

		if (!emulate_vtlb_is_directory_entry(vtlb, j, entry_address)) {
			walk_address = emulate_vtlb_allocate_directory(vtlb, j+1);
			if (walk_address == VTLB_ALLOCATOR_EMPTY) {
				emulate_vtlb_evict_on_level(vtlb, j+1);
				walk_address = emulate_vtlb_allocate_directory(vtlb, j+1);
			}
			emulate_vtlb_make_directory_entry(vtlb, j, entry_address, walk_address);
		}
		walk_address = emulate_vtlb_enter_directory(vtlb, j, entry_address);
	}

	emulate_vtlb_make_mapping_entry(vtlb, level, entry_address, paddr, flags);
}

// FLUSHING

static int emulate_vtlb_is_phidias_area(uint32_t level, uint32_t iter) {
	return (level == 0) && ((iter << 2) >= 0xf400) && ((iter << 2) < 0xf800);
}

static void emulate_vtlb_flush_slot_level(emulate_vtlb *vtlb, uint32_t i, uint32_t level, uintptr_t base) {
	uint32_t j;
	uintptr_t base_virt = emulate_vtlb_pool_vaddr(vtlb, level, base);

	for (j = 0; j < (1U << vtlb->level_pools[level].dirsize_log2); j += (1U << vtlb->level_pools[level].entrysize_log2)) {
		if (vtlb->is_paravirt && emulate_vtlb_is_phidias_area(level, j))
			continue;

		if (emulate_vtlb_is_directory_entry(vtlb, level, base_virt + j)) {
			uint32_t directory = emulate_vtlb_enter_directory(vtlb, level, base_virt + j);
			emulate_vtlb_flush_slot_level(vtlb, i, level+1, directory);
			emulate_vtlb_return_directory(vtlb, level+1, directory);
		}

		*(uint32_t *)(base_virt + j) = 0;
	}
}

static void emulate_vtlb_flush_slot(emulate_vtlb *vtlb, uint32_t i) {
	printf("VTLB Flush Slot %d\r\n", i);
	emulate_vtlb_flush_slot_level(vtlb, i, 0, vtlb->instances[i].pagetable_address);

	tlb_flush(TLB_ALL_ADDRESSES, vtlb->instances[i].identifier);
}

void emulate_vtlb_flush_by_identifier(const vm_cpu *vm_cpu_flush, uint32_t identifier) {
	emulate_vtlb *vtlb = vm_cpu_flush->vtlb_emulate->control.vtlb;
	uint32_t i;

	for (i = 0; i < vtlb->num_instances; i++) {
		if (vtlb->instances[i].vm_identifier == identifier) {
			emulate_vtlb_flush_slot(vtlb, i);
			return;
		}
	}
}

void emulate_vtlb_flush_current(const vm_cpu *vm_cpu_flush) {
	emulate_vtlb *vtlb = vm_cpu_flush->vtlb_emulate->control.vtlb;

	emulate_vtlb_flush_slot(vtlb, vtlb->active_instance);
}

void emulate_vtlb_flush_all(const vm_cpu *vm_cpu_flush) {
	uint32_t i;
	struct emulate_vtlb_ *vtlb = vm_cpu_flush->vtlb_emulate->control.vtlb;

	for (i = 0; i < vtlb->num_instances; i++)
		emulate_vtlb_flush_slot(vtlb, i);
}

// SELECTIVE UNMAP

static void emulate_vtlb_unmap_slot_level_address(emulate_vtlb *vtlb, uint32_t i, uint32_t level,
				uintptr_t base, uintptr_t address) {
	uintptr_t base_virt = emulate_vtlb_pool_vaddr(vtlb, level, base);
	uint32_t index = address >> vtlb->level_pools[level].shift;

	index &= (1U << (vtlb->level_pools[level].dirsize_log2 - vtlb->level_pools[level].entrysize_log2)) - 1;
	index <<= vtlb->level_pools[level].entrysize_log2;

	if (emulate_vtlb_is_directory_entry(vtlb, level, base_virt + index)) {
		uint32_t directory = emulate_vtlb_enter_directory(vtlb, level, base_virt + index);
		emulate_vtlb_unmap_slot_level_address(vtlb, i, level+1, directory, address);
		return;
	} else {
		*(uint32_t *)(base_virt + index) = 0;
		tlb_flush(address, vtlb->instances[i].identifier);
	}
}

static void emulate_vtlb_unmap_slot(emulate_vtlb *vtlb, uint32_t i, uintptr_t address) {
	emulate_vtlb_unmap_slot_level_address(vtlb, i, 0, vtlb->instances[i].pagetable_address, address);
}

void emulate_vtlb_unmap_by_identifier(const vm_cpu *vm_cpu_flush, uint32_t vm_identifier, uintptr_t address) {
	emulate_vtlb *vtlb = vm_cpu_flush->vtlb_emulate->control.vtlb;
	uint32_t i;

	for (i = 0; i < vtlb->num_instances; i++) {
		if (vtlb->instances[i].vm_identifier == vm_identifier) {
			emulate_vtlb_unmap_slot(vtlb, i, address);
			return;
		}
	}
}

// SWITCHER

void emulate_vtlb_switch(const vm_cpu *vm_cpu_switch, uintptr_t vm_pagetable_address, uint32_t vm_identifier) {
	emulate_vtlb *vtlb = vm_cpu_switch->vtlb_emulate->control.vtlb;
	uint32_t i;
	uint32_t i_oldest, i_free;
	uint64_t usage_oldest = ~0UL;

	i_free = i_oldest = vtlb->num_instances;

	for (i = 0; i < vtlb->num_instances; i++) {
		if (vtlb->instances[i].vm_identifier == vm_identifier) {
			vtlb->instances[i].vm_pagetable_address = vm_pagetable_address;
			emulate_vtlb_activate_slot(vm_cpu_switch, vtlb, i);
			return;
		}
		if (vtlb->instances[i].vm_identifier == VTLB_INVALID_IDENTIFIER) {
			i_free = i;
		} else if (vtlb->instances[i].last_usage < usage_oldest) {
			usage_oldest = vtlb->instances[i].last_usage;
			i_oldest = i;
		}
	}

	if (i_free != vtlb->num_instances) {
		vtlb->instances[i_free].vm_pagetable_address = vm_pagetable_address;
		vtlb->instances[i_free].vm_identifier = vm_identifier;
		emulate_vtlb_activate_slot(vm_cpu_switch, vtlb, i_free);
		return;
	}

	emulate_vtlb_flush_slot(vtlb, i_oldest);
	// @@@ asm volatile("mcr p15, 0, r0, c8, c7, 0" ::: "memory");
	vtlb->instances[i_oldest].vm_pagetable_address = vm_pagetable_address;
	vtlb->instances[i_oldest].vm_identifier = vm_identifier;
	emulate_vtlb_activate_slot(vm_cpu_switch, vtlb, i_oldest);
}
