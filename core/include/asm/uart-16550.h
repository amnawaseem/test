#ifndef	_PHIDIAS_ASM_UART_16550_H_
#define	_PHIDIAS_ASM_UART_16550_H_

# define	U16550_RBR	0x00 /* r */
# define	U16550_THR	0x00 /* w */
# define	U16550_IER	0x04
# define	U16550_IIR	0x08 /* r */
# define	U16550_FCR	0x08 /* w */
# define	U16550_LCR	0x0c
# define	U16550_MCR	0x10
# define	U16550_LSR	0x14
# define	U16550_MSR	0x18
# define	U16550_SCRATCH	0x1c

# define	U16550BIT_IIR_RDI	0x04

# define	U16550BIT_LSR_TEMT	0x40
# define	U16550BIT_LSR_THRE	0x20

#endif
