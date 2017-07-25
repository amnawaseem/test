#ifndef	__ASSEMBLER__

struct vm_cpu_;
struct emulate_;
struct emulate_vtlb_;

extern void vm_cpu_initialize(const struct vm_cpu_ *);

extern void __attribute__((noreturn)) vm_cpu_upcall(void);

extern void vm_cpu_state_save_full(const struct vm_cpu_ *);
extern void vm_cpu_state_load_full(const struct vm_cpu_ *);

extern void vm_cpu_emulate_faulted(const struct vm_cpu_ *, const struct emulate_ *, uintptr_t);

/* VM Event Injection */

extern void vm_cpu_inject_undefined(const struct vm_cpu_ *vm_cpu_inject);
extern void vm_cpu_inject_supervisorcall(const struct vm_cpu_ *vm_cpu_inject);
extern void vm_cpu_inject_fault(const struct vm_cpu_ *vm_cpu_inject, uintptr_t address, uint32_t flags);
extern void vm_cpu_inject_interrupt(const struct vm_cpu_ *vm_cpu_inject, uint32_t interrupt_number);

/* VTLB Architecture Interface: Part 1 (VTLB management) */

extern int emulate_vtlb_is_directory_entry(struct emulate_vtlb_ *vtlb, uint32_t level, uintptr_t entry_address);
extern uintptr_t emulate_vtlb_enter_directory(struct emulate_vtlb_ *vtlb, uint32_t level, uintptr_t entry_address);

extern void emulate_vtlb_make_directory_entry(struct emulate_vtlb_ *vtlb, uint32_t level,
		uintptr_t entry_address, uintptr_t directory_address);
extern void emulate_vtlb_make_mapping_entry(struct emulate_vtlb_ *vtlb, uint32_t level,
		uintptr_t entry_address, uintptr_t directory_address, uint32_t flags);

/* VTLB Architecture Interface: Part 2 (guest table walker) */

extern uintptr_t emulate_vtlb_walk(const struct vm_cpu_ *, uintptr_t address, uintptr_t *base, uintptr_t *size, uint32_t *flags);

extern void emulate_vtlb_ensure_host_paging(void);
extern void emulate_vtlb_ensure_guest_paging(const struct vm_cpu_ *);

#endif
