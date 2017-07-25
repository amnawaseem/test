#include <phidias.h>
#include <interrupts.h>
#include <drivers/timer.h>
#include <drivers/uart.h>

/**
 * Supported interrupts on the Raspberry Pi 3:
 * 0x1a		EL2 timer
 * 0x61		UART
 */
irqhandler_function * const irqhandlers[IRQID_MAXIMUM] = {
	0, 0, 0, 0, 0, 0, 0, 0,		/* SGIs (00-0f) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* PPIs (10-1f) */
	0, 0, &timer_interrupt, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* GPU IRQs PendingBase 00-07 (20-27) */

	0, 0, 0, 0, 0, 0, 0, 0,		/* GPU IRQs Pending1 00-0f (28-37) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* GPU IRQs Pending1 10-1f (38-47) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* GPU IRQs Pending2 00-0f (48-57) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* GPU IRQs Pending2 10-1f (58-67) */
	0, &uart_irq, 0, 0, 0, 0, 0, 0,
};
