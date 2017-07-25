#include <phidias.h>
#include <interrupts.h>
#include <xcore.h>
#include <drivers/timer.h>
#include <drivers/uart.h>
#include <emulate/timer.h>

/**
 * Supported interrupts on the HiKey (2GB):
 */
irqhandler_function * const irqhandlers[IRQID_MAXIMUM] = {
	0, 0, 0, 0, &xcore_ipi_interrupt, 0, 0, 0,		/* SGIs (00-0f) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* PPIs (10-1f) */
	0, 0, &timer_interrupt, &emulate_timer_armcp14_interrupt, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* SPIs (20-...) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* (30-...) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, &uart_irq,	/* (40-...) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* (50-...) */
	0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0,		/* (60-...) */
	0, 0, 0, 0, 0, 0, 0, 0,
};
