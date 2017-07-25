#ifndef	_PHIDIAS_ARCH_X86_SPECIFICATION_H_
#define	_PHIDIAS_ARCH_X86_SPECIFICATION_H_

struct specification_arch_cpu_;

struct specification_arch_ {
	const struct specification_arch_cpu_	*cpus;
	void		*gdt;
	void		*tss;
};
typedef struct specification_arch_	specification_arch;

struct specification_arch_cpu_ {
	uint32_t	_rsvd;
};
typedef struct specification_arch_cpu_	specification_arch_cpu;

extern uint32_t _incoming_idt_vector;

#endif
