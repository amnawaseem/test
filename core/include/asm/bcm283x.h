#ifndef	_PHDIAS_ASM_BCM283X_H_
#define	_PHDIAS_ASM_BCM283X_H_

/* definitions for the BCM2835 IRQ controller mapped at 0x3f00b/12 */

# define	BCM2835_IRQ_PENDINGB		0x200
# define	BCM2835_IRQ_PENDING1		0x204
# define	BCM2835_IRQ_PENDING2		0x208

# define	BCM2835_IRQ_ENABLE1		0x210
# define	BCM2835_IRQ_ENABLE2		0x214
# define	BCM2835_IRQ_ENABLEB		0x218

# define	BCM2835_IRQ_DISABLE1		0x21c
# define	BCM2835_IRQ_DISABLE2		0x220
# define	BCM2835_IRQ_DISABLEB		0x224

# define	BCM2835_IRQ_ERRHLT		0x448

/* definitions for the mailbox control area at 0x40000/12 */

# define	BCM2836_IRQ_TIMERCTL(cpu)	(0x040 + (cpu<<2))
# define	BCM2836_IRQ_MAILBOXCTL(cpu)	(0x050 + (cpu<<2))
# define	BCM2836_IRQ_IRQSOURCE(cpu)	(0x060 + (cpu<<2))
# define	BCM2836_IRQ_FIQSOURCE(cpu)	(0x070 + (cpu<<2))

# define	BCM2836_IRQ_MAILBOX_WRITE(cpu, box)	(0x080 + (box<<2) + (cpu<<4))
# define	BCM2836_IRQ_MAILBOX_READ(cpu, box)	(0x0c0 + (box<<2) + (cpu<<4))
# define	BCM2836_IRQ_MAILBOX_CLEAR(cpu, box)	BCM2835_IRQ_MAILBOX_READ(cpu, box)

#endif
