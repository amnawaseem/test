#ifndef	_PHIDIAS_EMULATE_MEMORY_H_
#define	_PHIDIAS_EMULATE_MEMORY_H_

#define	EMULATE_MEMORY_FLAG_SINGLE_VALUE	0x00000001
#define	EMULATE_MEMORY_FLAG_SINGLE_MEM_RMASK	0x00000010
#define	EMULATE_MEMORY_FLAG_SINGLE_MEM_WMASK	0x00000100
#define	EMULATE_MEMORY_FLAG_SINGLE_HW_RMASK	0x00001000
#define	EMULATE_MEMORY_FLAG_SINGLE_HW_WMASK	0x00010000

struct emulate_memory_ {
	uint32_t			flags;
	uint32_t			value_default;
	uint32_t			mem_rmask_default;
	uint32_t			mem_wmask_default;
	uint32_t			hw_rmask_default;
	uint32_t			hw_wmask_default;
	uint32_t			*memory;
	volatile uint32_t		*hw;
	const uint32_t			*mem_rmask;
	const uint32_t			*mem_wmask;
	const uint32_t			*hw_rmask;
	const uint32_t			*hw_wmask;
};
typedef struct emulate_memory_	emulate_memory;

struct vm_cpu_;

void emulate_memory_initialize(const struct vm_cpu_ *vm_cpu_reset, const emulate_memory *memory);

void emulate_memory_store(const struct vm_cpu_ *vm_cpu_em, const emulate_memory *memory,
                uint32_t bar, uintptr_t offset, uint32_t value);
uint32_t emulate_memory_load(const struct vm_cpu_ *vm_cpu_em, const emulate_memory *memory,
                uint32_t bar, uintptr_t offset);

#endif
