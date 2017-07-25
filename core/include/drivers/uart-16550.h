#ifndef	_PHIDIAS_DRIVERS_UART_16550_H_
#define	_PHIDIAS_DRIVERS_UART_16550_H_

# include <asm/uart-16550.h>

struct memarea_;

void uart_16550_setup(const struct memarea_ *area);
void uart_16550_putchar(const struct memarea_ *area, uint8_t ch);
void uart_16550_putstring(const struct memarea_ *area, const char *string);
uint32_t uart_16550_irq(const struct memarea_ *area);

# if defined(CONFIG_TWEAK_UART_16550_IOPORT_3F8)
#  define UART_16550_IOPORTNO	0x3f8
# elif defined(CONFIG_TWEAK_UART_16550_IOPORT_2F8)
#  define UART_16550_IOPORTNO	0x2f8
# endif

# if defined(UART_16550_IOPORTNO)

static inline uint32_t _u16550_read(const memarea *area, uint32_t off) {
	uint32_t val;

	(void)area;
	asm volatile("inb (%%dx), %%al\n\tmovzbl %%al, %%eax" : "=a" (val) : "d" (UART_16550_IOPORTNO + (off>>2)));

	return val;
}

static inline void _u16550_write(const memarea *area, uint32_t off, uint32_t val) {
	(void)area;
	asm volatile("outb %%al, (%%dx)" :: "a" (val), "d" (UART_16550_IOPORTNO + (off>>2)));
}

# else

static inline uint32_t _u16550_read(const memarea *area, uint32_t off) {
	uint32_t val;

	val = mmio_read32(area->vaddr + off);

	return val;
}

static inline void _u16550_write(const memarea *area, uint32_t off, uint32_t val) {
	mmio_write32(area->vaddr + off, val);
}

# endif

#endif
