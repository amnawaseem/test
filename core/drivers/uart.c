#include <phidias.h>
#include <memarea.h>

uint32_t uart_irq() {
#if defined(UART_DRIVER_PL011)
	return uart_pl011_irq(core_memarea(MEMAREA_SERIAL));
#elif defined(UART_DRIVER_16550)
	return uart_16550_irq(core_memarea(MEMAREA_SERIAL));
#elif defined(UART_DRIVER_PIC32)
	return uart_pic32_irq(core_memarea(MEMAREA_SERIAL));
#else
# warning No uart driver selected!
#endif
}
