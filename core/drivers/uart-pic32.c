#include <phidias.h>
#include <mmio.h>
#include <drivers/uart.h>
#include <lock.h>
#include <schedule.h>
#include <emulate/uart.h>
#include <multiplexer.h>

uint32_t __shared pic32_lock;

/**
 * Enables the internal UART.
 */
void uart_pic32_setup(const memarea *area) {
	while (!(mmio_read32(area->vaddr + PIC32REG_UXSTA) & PIC32BIT_UXTRMT) ||
		!(mmio_read32(area->vaddr + PIC32REG_UXSTA) & PIC32BIT_UXRIDLE))
		;

	/* possible barrier */
	mmio_write32(area->vaddr + PIC32REG_UXBRG, 53);
	mmio_write32(area->vaddr + PIC32REG_UXSTA, PIC32BIT_UXURXEN | PIC32BIT_UXUTXEN);
	mmio_write32(area->vaddr + PIC32REG_UXMODE, PIC32BIT_UXON);

	/* PPS config */
	mmio_write32(PIC32REG_RPD4R, 2);
	mmio_write32(PIC32REG_U4RXR, 3);
	/* possible barrier */
	spinlock_unlock(&pic32_lock);
}

/**
 * Busywait for the UART to become ready for transmission again.
 * This should be safe even with enabled FIFO, as we wait for the
 * FifoFull bit to clear (instead of the TX empty bit to appear).
 */
static void uart_pic32_poll_txready(const memarea *area) {
	while (mmio_read32(area->vaddr + PIC32REG_UXSTA) & PIC32BIT_UXUTXBF)
		;
}

static inline void uart_pic32_putchar_UL(const memarea *area, uint8_t ch) {
	uart_pic32_poll_txready(area);
	
	mmio_write32(area->vaddr + PIC32REG_UXTXREG, ch);
}

/**
 * Transmit a single character.
 */
void uart_pic32_putchar(const memarea *area, uint8_t ch) {
	spinlock_lock(&pic32_lock);
	uart_pic32_putchar_UL(area, ch);
	spinlock_unlock(&pic32_lock);
}

/**
 * Transmit a string.
 */
void uart_pic32_putstring(const memarea *area, const char *string) {
	spinlock_lock(&pic32_lock);

	while (*string) {
		uart_pic32_putchar_UL(area, *string);
		++string;
	}

	spinlock_unlock(&pic32_lock);
}

/**
 * React to an RX interrupt: grab the character(s) from the FIFO.
 */
uint32_t __attribute__ (( section(".isr"), interrupt("eic"))) uart_pic32_irq(const memarea *area) {
	while (mmio_read32(area->vaddr + PIC32REG_UXSTA) & PIC32BIT_UXURXDA) {
		uint8_t ch = (uint8_t)mmio_read32(area->vaddr + PIC32REG_UXRXREG);
		(void)ch;
		//emulate_uart_push_character(current_vm_cpu, ch);
#ifdef FEATURE_MULTIPLEXER
		mux_in_char(ch);
#endif
	}

	return 0;
}
