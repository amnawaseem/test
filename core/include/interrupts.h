#ifndef	_PHIDIAS_INTERRUPTS_H_
#define	_PHIDIAS_INTERRUPTS_H_

struct vm_;

# define	IRQID_SPURIOUS	0xffffffff

# if defined(ARCH_X86)
#  define	IRQID_MAXIMUM	256
# elif defined(ARCH_ARM) || defined(ARCH_ARM64)
#  define	IRQID_MAXIMUM	1024
# elif defined(ARCH_MIPS)
#  define	IRQID_MAXIMUM	213
# endif

# define	IRQ_KEEP_MASKED		0x0001

typedef uint32_t (irqhandler_function)(void);

extern void interrupt_handler(void);
extern void deliver_vm_ipi(const struct vm_ *target_vm, uint32_t vcpu_no, uint32_t interrupt);
extern void deliver_irq_to_vm(const struct vm_ *target_vm, uint32_t interrupt_number, uint32_t is_passthrough);

extern irqhandler_function * const irqhandlers[IRQID_MAXIMUM];

#endif
