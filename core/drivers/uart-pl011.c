#include <phidias.h>
#include <mmio.h>
#include <drivers/uart.h>
#include <lock.h>
#include <schedule.h>
#include <emulate/uart.h>
#include <specification.h>
#include <vm.h>
#include <multiplexer.h>

uint32_t __shared pl011_lock;

/**
 * Enables the PL011.
 */
void uart_pl011_setup(const memarea *area) {
	while (!(mmio_read32(area->vaddr + PL011REG_UARTFR) & PL011BIT_UARTFR_TXFE) ||
		(mmio_read32(area->vaddr + PL011REG_UARTFR) & PL011BIT_UARTFR_BUSY))
		;

	asm volatile("dsb sy" ::: "memory");
	mmio_write32(area->vaddr + PL011REG_UARTLCR, PL011BIT_UARTLCR_WLEN8 | PL011BIT_UARTLCR_FEN);
	mmio_write32(area->vaddr + PL011REG_UARTCR, PL011BIT_UARTCR_EN | PL011BIT_UARTCR_TXEN | PL011BIT_UARTCR_RXEN);
	mmio_write32(area->vaddr + PL011REG_UARTIFLS, 0);
	mmio_write32(area->vaddr + PL011REG_UARTIMSC, PL011BIT_UARTIRQ_RX | PL011BIT_UARTIRQ_RT);
	mmio_write32(area->vaddr + PL011REG_UARTICR, PL011BIT_UARTIRQ_RX | PL011BIT_UARTIRQ_RT);
	asm volatile("dsb sy" ::: "memory");
}

/**
 * Busywait for the PL011 to become ready for transmission again.
 * This should be safe even with enabled FIFO, as we wait for the
 * FifoFull bit to clear (instead of the TX empty bit to appear).
 */
static void uart_pl011_poll_txready(const memarea *area) {
	while (mmio_read32(area->vaddr + PL011REG_UARTFR) & PL011BIT_UARTFR_TXFF)
		;
}

static inline void uart_pl011_putchar_UL(const memarea *area, uint8_t ch) {
	uart_pl011_poll_txready(area);

	mmio_write32(area->vaddr + PL011REG_UARTDR, ch);
}

/**
 * Transmit a single character over the PL011.
 */
void uart_pl011_putchar(const memarea *area, uint8_t ch) {
	spinlock_lock(&pl011_lock);
	uart_pl011_putchar_UL(area, ch);
	spinlock_unlock(&pl011_lock);
}

/**
 * Transmit a string over the PL011.
 */
void uart_pl011_putstring(const memarea *area, const char *string) {
	spinlock_lock(&pl011_lock);

	while (*string) {
		uart_pl011_putchar_UL(area, *string);
		++string;
	}

	spinlock_unlock(&pl011_lock);
}

/**
 * React to an RX interrupt: grab the character(s) from the FIFO.
 */
uint32_t uart_pl011_irq(const memarea *area) {
	uint32_t reason = mmio_read32(area->vaddr + PL011REG_UARTMIS);

	if (reason & (PL011BIT_UARTIRQ_RX | PL011BIT_UARTIRQ_RT)) {
		while (!(mmio_read32(area->vaddr + PL011REG_UARTFR) & PL011BIT_UARTFR_RXFE)) {
			uint8_t ch = (uint8_t)mmio_read32(area->vaddr + PL011REG_UARTDR);
#ifdef FEATURE_MULTIPLEXER
			mux_in_char(ch);
#else
			if (current_vm_cpu != NULL) {
				emulate_uart_push_character(current_vm_cpu, ch);
			} else
				emulate_uart_push_character((void *)0xf4015c20, ch);
#endif
		}
	}

	// mmio_write32(area->vaddr + PL011REG_UARTICR, reason);

	return 0;
}
