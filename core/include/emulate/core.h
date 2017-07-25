#ifndef	_PHIDIAS_EMULATE_CORE_H_
#define	_PHIDIAS_EMULATE_CORE_H_

struct vm_cpu_;
struct emulate_memory_;
struct emulate_vtlb_;
struct emulate_uart_pl011_;

#define	EMULATE_TYPE_VTLB		0x00010000
#define	EMULATE_TYPE_UART_PL011		0x00020001
#define	EMULATE_TYPE_UART_16550		0x00020002
#define	EMULATE_TYPE_UART_PHIDIAS	0x00020003
#define	EMULATE_TYPE_UART_PIC32		0x00020004
#define	EMULATE_TYPE_TIMER_MPCORE	0x00030001
#define	EMULATE_TYPE_TIMER_SP804	0x00030002
#define	EMULATE_TYPE_TIMER_ARMCP14	0x00030003
#define	EMULATE_TYPE_CLOCK_MPCORE	0x00040001
#define	EMULATE_TYPE_IRQ_MPCORE		0x00050001
#define	EMULATE_TYPE_IRQ_GIC		0x00050002
#define	EMULATE_TYPE_IRQ_GIC_VIRTEXT	0x00050012
#define	EMULATE_TYPE_IRQ_LAPIC		0x00050003
#define	EMULATE_TYPE_MEMORY_32BIT	0x00060001

union emulate_control_ {
	const void				*_void;
	struct emulate_vtlb_			*vtlb;
	struct emulate_uart_pl011_		*uart_pl011;
	struct emulate_uart_16550_		*uart_16550;
	struct emulate_uart_pic32_		*uart_pic32;
	struct emulate_timer_sp804_		*timer_sp804;
	struct emulate_timer_mpcore_		*timer_mpcore;
	struct emulate_timer_armcp14_		*timer_armcp14;
	struct emulate_irq_gic_			*irq_gic;
	struct emulate_irq_lapic_		*irq_lapic;
	const struct emulate_memory_		*memory;
};

/**
 * This structure contains the control data for a single emulated region.
 * For every possible type, the union contains a corresponding structure
 * pointer.
 */
struct emulate_ {
	uintptr_t			vaddr;
	uintptr_t			size;
	uint32_t			type;

	union emulate_control_		control;
	uint32_t			bar;
};
typedef struct emulate_			emulate;

struct tree_emulate_ {
	const struct tree_emulate_	*left;
	const struct tree_emulate_	*right;
	const struct emulate_		*item;
};
typedef struct tree_emulate_	tree_emulate;

extern const struct emulate_ *find_emulate_by_va(const tree_emulate *tree, uintptr_t vaddr);

extern void emulate_initialize_all(const struct vm_cpu_ *);
extern uint32_t emulate_peripheral_load(const struct vm_cpu_ *vm_cpu_em, const emulate *emul, uintptr_t address);
extern void emulate_peripheral_store(const struct vm_cpu_ *vm_cpu_em, const emulate *emul, uintptr_t address, uint32_t value);

extern void vm_cpu_save_emulates(const struct vm_cpu_ *);
extern void vm_cpu_load_emulates(const struct vm_cpu_ *);

#endif
