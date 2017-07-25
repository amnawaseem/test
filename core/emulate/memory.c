#include <phidias.h>
#include <vm.h>
#include <emulate/core.h>
#include <emulate/memory.h>

void emulate_memory_initialize(const vm_cpu *vm_cpu_reset, const emulate_memory *memory) {
	(void)vm_cpu_reset; (void)memory;
}

void emulate_memory_store(const vm_cpu *vm_cpu_em, const emulate_memory *memory,
		uint32_t bar, uintptr_t offset, uint32_t value) {
	uint32_t hw_mask;
	uint32_t mem_mask;
	uint32_t new_value;

	(void)vm_cpu_em; (void)bar;

	if (memory->flags & EMULATE_MEMORY_FLAG_SINGLE_MEM_WMASK) {
		mem_mask = memory->mem_wmask_default;
	} else {
		mem_mask = memory->mem_wmask[offset >> 2];
	}

	if (memory->flags & EMULATE_MEMORY_FLAG_SINGLE_HW_WMASK) {
		hw_mask = memory->hw_wmask_default;
	} else {
		hw_mask = memory->hw_wmask[offset >> 2];
	}

	if (hw_mask) {
		new_value = memory->hw[offset >> 2] & ~hw_mask;
		new_value |= value & hw_mask;
		memory->hw[offset >> 2] = new_value;
	}

	if (mem_mask) {
		new_value = memory->memory[offset >> 2] & ~mem_mask;
		new_value |= value & mem_mask;
		memory->memory[offset >> 2] = new_value;
	}
}

uint32_t emulate_memory_load(const vm_cpu *vm_cpu_em, const emulate_memory *memory,
		uint32_t bar, uintptr_t offset) {
	uint32_t hw_mask;
	uint32_t mem_mask;
	uint32_t mem_value;
	uint32_t return_value;

	(void)vm_cpu_em; (void)bar;

	if (memory->flags & EMULATE_MEMORY_FLAG_SINGLE_MEM_RMASK) {
		mem_mask = memory->mem_rmask_default;
	} else {
		mem_mask = memory->mem_rmask[offset >> 2];
	}

	if (memory->flags & EMULATE_MEMORY_FLAG_SINGLE_HW_RMASK) {
		hw_mask = memory->hw_rmask_default;
	} else {
		hw_mask = memory->hw_rmask[offset >> 2];
	}

	if (memory->flags & EMULATE_MEMORY_FLAG_SINGLE_VALUE) {
		mem_value = memory->value_default;
	} else {
		mem_value = memory->memory[offset >> 2];
	}

	return_value = 0;

	if (hw_mask) {
		return_value |= memory->hw[offset >> 2] & hw_mask;
	}

	if (mem_mask) {
		return_value |= mem_value & mem_mask;
	}

	return return_value;
}
