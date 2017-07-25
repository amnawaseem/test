#ifndef	_PHIDIAS_DEBUG_H_
#define	_PHIDIAS_DEBUG_H_

# ifdef	FEATURE_DEBUG

#  include <drivers/uart.h>
#include <lock.h>
#include <multiplexer.h>


void printf(const char *format, ...);

#  if	WORDSIZE == 32

static inline uint64_t get_64bit_argument(void *vararg_arg, uint32_t *counter) {
	__builtin_va_list *varargs = (__builtin_va_list *)vararg_arg;
	uint64_t value;

	if (*counter & 1) {
		(void)__builtin_va_arg(*varargs, uint32_t);
		++(*counter);
	}

	value = __builtin_va_arg(*varargs, uint32_t);
	value |= ((uint64_t)__builtin_va_arg(*varargs, uint32_t)) << 32;

	*counter += 2;

	return value;
}

#  elif	WORDSIZE == 64

static inline uint64_t get_64bit_argument(void *vararg_arg, uint32_t *counter) {
	__builtin_va_list *varargs = (__builtin_va_list *)vararg_arg;
	uint64_t value;

	value = __builtin_va_arg(*varargs, uint64_t);
	++(*counter);

	return value;
}

#  endif




static inline void debug_putchar(uint8_t ch) {
#ifdef FEATURE_MULTIPLEXER
	mux_out_char(ch,0);
#else
	uart_putchar(ch);
#endif
}

static inline void debug_putstring(const char *string) {
#ifdef FEATURE_MULTIPLEXER
	mux_out_string(string);
#else
	uart_putstring(string);
#endif
}

# else	/* !(FEATURE_DEBUG) */

# define printf(fmt,...)
# define putchar(ch)
# define putstring(str)

# endif

#endif
