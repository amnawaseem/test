#ifndef	_PHIDIAS_DRIVERS_UART_H_
#define	_PHIDIAS_DRIVERS_UART_H_

# include <memarea.h>

# if defined(UART_DRIVER_PL011)

#  include <drivers/uart-pl011.h>

static inline void uart_setup(void) {
	uart_pl011_setup(core_memarea(MEMAREA_SERIAL));
}

static inline void uart_putchar(uint8_t ch) {
	uart_pl011_putchar(core_memarea(MEMAREA_SERIAL), ch);
}

static inline void uart_putstring(const char *string) {
	uart_pl011_putstring(core_memarea(MEMAREA_SERIAL), string);
}

# elif defined(UART_DRIVER_16550)

#  include <drivers/uart-16550.h>

static inline void uart_setup(void) {
	uart_16550_setup(core_memarea(MEMAREA_SERIAL));
}

static inline void uart_putchar(uint8_t ch) {
	uart_16550_putchar(core_memarea(MEMAREA_SERIAL), ch);
}

static inline void uart_putstring(const char *string) {
	uart_16550_putstring(core_memarea(MEMAREA_SERIAL), string);
}

# elif defined(UART_DRIVER_PIC32)

#  include <drivers/uart-pic32.h>

static inline void uart_setup(void) {
	uart_pic32_setup(core_memarea(MEMAREA_SERIAL));
}

static inline void uart_putchar(uint8_t ch) {
	uart_pic32_putchar(core_memarea(MEMAREA_SERIAL), ch);
}

static inline void uart_putstring(const char *string) {
	uart_pic32_putstring(core_memarea(MEMAREA_SERIAL), string);
}

# endif

extern uint32_t uart_irq(void);

#endif
