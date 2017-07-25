#include <phidias.h>
#include <mmio.h>
#include <drivers/uart.h>
#include <lock.h>
#include <schedule.h>
#include <emulate/uart.h>
#include <multiplexer.h>

uint32_t __shared u16550_lock;

/**
 * Enables the 16550.
 */
void uart_16550_setup(const memarea *area) {
	_u16550_write(area, U16550_IER, 0x01);
}

/**
 * Busywait for the 16550 to become ready for transmission again.
 */
static void uart_16550_poll_txready(const memarea *area) {
	while (!(_u16550_read(area, U16550_LSR) & U16550BIT_LSR_THRE))
		;
}

static inline void uart_16550_putchar_UL(const memarea *area, uint8_t ch) {
	uart_16550_poll_txready(area);

	_u16550_write(area, U16550_THR, ch);
}

/**
 * Transmit a single character over the 16550.
 */
void uart_16550_putchar(const memarea *area, uint8_t ch) {
	spinlock_lock(&u16550_lock);
	uart_16550_putchar_UL(area, ch);
	spinlock_unlock(&u16550_lock);
}

/**
 * Transmit a string over the 16550.
 */
void uart_16550_putstring(const memarea *area, const char *string) {
	spinlock_lock(&u16550_lock);

	while (*string) {
		uart_16550_putchar_UL(area, *string);
		++string;
	}

	spinlock_unlock(&u16550_lock);
}

/**
 * React to an RX interrupt: grab the character(s) from the FIFO.
 */
uint32_t uart_16550_irq(const memarea *area) {
	uint32_t reason = _u16550_read(area, U16550_IIR);
	uint8_t ch;

	switch (reason) {
	case U16550BIT_IIR_RDI:
		ch = _u16550_read(area, U16550_RBR);
		// printf("UART ChIn %x\r\n", ch);
		// send char to current VCPU, or the first one on this core if we are idle
#ifdef FEATURE_MULTIPLEXER
		mux_in_char(ch);
#else
		emulate_uart_push_character(current_vm_cpu ?: _specification.cpus[cpu_number].vm_cpus[0], ch);
#endif
		break;
	default:
		printf("UART IRQ %x (unhandled reason)\r\n", reason);
		break;
	}

	return 0;
}
