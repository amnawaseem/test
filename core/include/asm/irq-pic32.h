#ifndef	_PHIDIAS_ASM_IRQ_PIC32_H_
#define	_PHIDIAS_ASM_IRQ_PIC32_H_

/* register and offset defines */
/* base 0xbf810000 */
#define	PIC32REG_INTCON		0x0000
#define	PIC32REG_PRISS		0x0010
#define	PIC32REG_INTSTAT	0x0020
#define	PIC32REG_IPTMR		0x0030
#define PIC32REG_IFS0		0x0040
#define PIC32REG_IFS1		0x0050
#define PIC32REG_IFS2		0x0060
#define PIC32REG_IFS3		0x0070
#define PIC32REG_IFS4		0x0080
#define PIC32REG_IFS5		0x0090
#define PIC32REG_IFS6		0x00A0
#define PIC32REG_IEC0		0x00C0
#define PIC32REG_IEC1		0x00D0
#define PIC32REG_IEC2		0x00E0
#define PIC32REG_IEC3		0x00F0
#define PIC32REG_IEC4		0x0100
#define PIC32REG_IEC5		0x0110
#define PIC32REG_IEC6		0x0120
#define PIC32REG_IPC2		0x0160
#define PIC32REG_IPC3		0x0170

#define PIC32REG_OFF14		0x0578
#define PIC32REG_OFF9		0x0564

/* bit offsets */
#define PIC32BIT_MVEC		0x1000
#define PIC32BIT_SS0		0x0001

#define PIC32IRQ_CORETIMER	0
#define PIC32IRQ_TIMER3		14
#define PIC32IRQ_TIMER5		24

#endif