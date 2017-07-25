#include <phidias.h>
#include <mmio.h>
#include <drivers/irq-bcm283x.h>
#include <asm/bcm283x.h>
#include <arch/fpu_support.h>

/*
 * This thing is completely non-standard, so we have to invent a crazy
 * numbering scheme to make it at least a little bit intuitive to use.
 * The first layer of IRQ delivery is the "ARM_LOCAL_PENDING" bitfield
 * on the 0x40000/12 page. If that one signals a "GPU" interrupt, we
 * move on to the second layer, the IRQ controller at 0x3f00b/12. There
 * the interrupt can either be in the "BASE PENDING" bitfield or, as
 * this one has cascading bits, too (8253 PIC, anyone?), in the "PENDING1"
 * or "PENDING2" additional bitfields. To make it even more complicated,
 * a few of the bits from "PENDING1" and "PENDING2" are shortcut to instead
 * appear in "BASE PENDING" (which we map back using base_irq_from_bit[]).
 *
 * Invented numbering:
 * 00-07	**unused**
 * 08-0b	BCM283x Inter-CPU Mailboxes
 * 0c-0f	**unused**
 * 10-19	**unused**
 * 1a		EL2 timer
 * 1b-1d	(other timers)
 * 1e-1f	**unused**
 * 20-27	the eight interrupts which belong to GPU "BASE PENDING"
 * 28-47	32 interrupt bits from GPU "PENDING1"
 * 48-67	32 interrupt bits from GPU "PENDING2"
 *
 * Ocassionally, an "Illegal Access Seen" interrupt might pop up at the GPU.
 * This can be cleared by just writing the status value back into the
 * Error/Halt register - a mechanism which is completely undocumented. :)
 *
 * IRQ sources as taken from the BCM2836-QA7 manual (rev 3.4) and from the
 * BCM2835 ARM Peripherals manual:
 * ARM_LOCAL 00	Secure EL1 Timer
 * ARM_LOCAL 01	Non-Secure EL1 Timer
 * ARM_LOCAL 02	EL2 Timer
 * ARM_LOCAL 03	Virtual Timer
 * ARM_LOCAL 04	Mailbox 0
 * ARM_LOCAL 05	Mailbox 1
 * ARM_LOCAL 06	Mailbox 2
 * ARM_LOCAL 07	Mailbox 3
 * ARM_LOCAL 08	Cascade (GPU) BASE
 * ARM_LOCAL 09	PMU
 * ARM_LOCAL 0a	AXI outstanding
 * ARM_LOCAL 0b	Local timer (SP804?)
 *
 * BASE 00	ARM Timer
 * BASE 01	ARM Mailbox
 * BASE 02	ARM Doorbell 0
 * BASE 03	ARM Doorbell 1
 * BASE 04	GPU0 halted
 * BASE 05	GPU1 halted
 * BASE 06	Illegal Access Type1 (
 * BASE 07	Illegal Access Type0 (
 * BASE 08	Cascade PENDING1
 * BASE 09	Cascade PENDING2
 * BASE 0a	GPU Peripheral 07
 * BASE 0b	GPU Peripheral 09
 * BASE 0c	GPU Peripheral 0a
 * BASE 0d	GPU Peripheral 12
 * BASE 0e	GPU Peripheral 13
 * BASE 0f	GPU Peripheral 35 (I2C)
 * BASE 10	GPU Peripheral 36 (SPI)
 * BASE 11	GPU Peripheral 37 (PCM)
 * BASE 12	GPU Peripheral 38
 * BASE 13	GPU Peripheral 39 (UART)
 * BASE 14	GPU Peripheral 3e
 *
 * PEND1 1d	AUX
 *
 * PEND2 0b	I2C SPI Slave
 * PEND2 0d	PWA0
 * PEND2 0e	PWA1
 * PEND2 10	SMI
 * PEND2 11	GPIO0
 * PEND2 12	GPIO1
 * PEND2 13	GPIO2
 * PEND2 14	GPIO3
 * PEND2 15	I2C
 * PEND2 16	SPI
 * PEND2 17	PCM
 * PEND2 19	UART
 */
void irq_bcm283x_setup(const memarea *area_irq, const memarea *area_mbox) {
	// enable all in "BASE" plus the UART in "2"
	mmio_write32(area_irq->vaddr + BCM2835_IRQ_ENABLE1, 0x00000000U);
	mmio_write32(area_irq->vaddr + BCM2835_IRQ_ENABLE2, 0x02000000U);	// UART
	mmio_write32(area_irq->vaddr + BCM2835_IRQ_ENABLEB, 0x000000ffU);

	// enable all four mailbox interrupts
	mmio_write32(area_mbox->vaddr + BCM2836_IRQ_MAILBOXCTL(cpu_number), 0xf);

	// enable all four ARM generic timer interrupts
	mmio_write32(area_mbox->vaddr + BCM2836_IRQ_TIMERCTL(cpu_number), 0xf);
}

static uint32_t base_irq_from_bit[] = {
	0x20, 0x21, 0x22, 0x23,
	0x24, 0x25, 0x3ff, 0x27,		// bit 6: Illegal Access, handled internally
	0x3ff, 0x3ff, 0x28+7, 0x28+9,		// bit 8,9: cascade
	0x28+10, 0x28+18, 0x28+19, 0x28+53,
	0x28+54, 0x28+55, 0x28+56, 0x28+57,
	0x28+62, 0x3ff, 0x3ff, 0x3ff,
	0x3ff, 0x3ff, 0x3ff, 0x3ff,
	0x3ff, 0x3ff, 0x3ff, 0x3ff
};

uint32_t irq_bcm283x_get_irq_raw(const memarea *area_irq, const memarea *area_mbox) {
	uint32_t purported_source = mmio_read32(area_mbox->vaddr + BCM2836_IRQ_IRQSOURCE(cpu_number));
	//printf("GetIrq: %x\r\n", purported_source);

	if (purported_source == 0) {
		return 0x3ff;
	}

	purported_source = highest_bit(purported_source);

	switch (purported_source) {
	case 8:				// 0x100: GPU IRQ
	{
		uint32_t base_pending = mmio_read32(area_irq->vaddr + BCM2835_IRQ_PENDINGB);

		if (base_pending & 0x40) {
			uint32_t err = mmio_read32(area_irq->vaddr + BCM2835_IRQ_ERRHLT);
			printf("GPU Err/Hlt: %x\r\n", err);
			mmio_write32(area_irq->vaddr + BCM2835_IRQ_ERRHLT, err);
			base_pending &= ~0x40;
		}

		if (base_pending & 0x100) {
			uint32_t ext1_pending = mmio_read32(area_irq->vaddr + BCM2835_IRQ_PENDING1);
			//printf("GPU Pend1: %x\r\n", ext1_pending);
			return 0x28 + highest_bit(ext1_pending);
		} else if (base_pending & 0x200) {
			uint32_t ext2_pending = mmio_read32(area_irq->vaddr + BCM2835_IRQ_PENDING2);
			//printf("GPU Pend2: %x\r\n", ext2_pending);
			return 0x48 + highest_bit(ext2_pending);
		} else if (base_pending) {
			//printf("GPU PendB: %x\r\n", base_pending);
			return base_irq_from_bit[highest_bit(base_pending)];
		}
		break;
	}
	case 2:				// 0x004: EL2 timer
		return 0x1a;
	default:
		printf("Unknown IRQ source\r\n");
	}

	return 0x3ff;
}

/*
 * On this strange platform, there is no such thing as an IRQ ACK.
 */
void irq_bcm283x_ack_irq(const memarea *area, uint32_t interrupt_number) {
	(void)area; (void)interrupt_number;
}

void irq_bcm283x_disable_irq(const memarea *area, uint32_t interrupt_number) {
	(void)area; (void)interrupt_number;
}
