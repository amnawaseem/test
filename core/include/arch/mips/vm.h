#ifndef	_PHIDIAS_ARCH_MIPS_VM_H_
#define	_PHIDIAS_ARCH_MIPS_VM_H_

struct vm_cpu_;
struct vm_cpu_state_;

extern void vm_cpu_initialize(const struct vm_cpu_ *new_vm_cpu);
extern void vm_cpu_dump_state(const struct vm_cpu_ *vm_cpu_x);

extern void vm_cpu_switch_bank(struct vm_cpu_state_ *state, uint32_t old_mode, uint32_t new_mode);
extern void vm_cpu_enter_vector(struct vm_cpu_state_ *state, uint32_t vector_offset);

#endif
