#include <phidias.h>
#include <misc_ops.h>
#include <arch/fpu_support.h>
#include <debug.h>

static void vprintf(const char *format, __builtin_va_list varargs);
static char *format_string(const char *str, uint32_t width);
static char *format_decimal(uint64_t value);
static char *format_hexadecimal(uint64_t value);
static char *pad_buffer(char *buffer, uint32_t width, uint8_t pad_char);


/**
 * Prints a debug message to Phidias' standard debugging stream (usually
 * the multiplexed master UART).
 *
 * This function is a simple wrapper around vprintf().
 *
 * \param[in]	format		Format string.
 * \sa		vprintf
 */
void printf(const char *format, ...) {
	__builtin_va_list varargs;

	__builtin_va_start(varargs, format);
	vprintf(format, varargs);
	__builtin_va_end(varargs);
}

/**
 * Formats a debug message and emits it through Phidias' standard debugging
 * stream.
 *
 * The following conversion specifiers are supported:
 *	%c		single character
 *	%s		null-terminated string
 *	%d		decimal number
 *	%x		hexadecimal number
 *
 * \param[in]	format		Format string.
 * \param[in]	varargs		Variadic parameter list.
 */
static void vprintf(const char *format, __builtin_va_list varargs) {
	uint32_t output_width;
	uint32_t argument_is_64bit;
	uint32_t argument_counter = 1;
	uint64_t value;
	char *buffer;
	char *str;

	for (; *format; ++format) {

		/* literal character */
		if (*format != '%') {
			debug_putchar(*format);
			continue;
		}

		/* we have a conversion */
		++format;
		argument_is_64bit = 0;
		output_width = 0;

		/* parse width if present */
		while ((*format >= '0') && (*format <= '9')) {
			output_width = (output_width * 10) + (*format - '0');
			++format;
		}

		if (*format == 'q') {
			argument_is_64bit = 1;
			++format;
		}

		switch (*format) {
		case '%':
			debug_putchar(*format);
			break;
		case 'd':
			if (argument_is_64bit) {
				value = get_64bit_argument(&varargs, &argument_counter);
			} else {
				value = __builtin_va_arg(varargs, wordsize_t);
				++argument_counter;
			}
			buffer = format_decimal(value);
			if (output_width) {
				buffer = pad_buffer(buffer, output_width, '0');
			}
			debug_putstring(buffer);
			break;
		case 'x':
			if (argument_is_64bit) {
				value = get_64bit_argument(&varargs, &argument_counter);
			} else {
				value = __builtin_va_arg(varargs, wordsize_t);
				++argument_counter;
			}
			buffer = format_hexadecimal(value);
			if (output_width) {
				buffer = pad_buffer(buffer, output_width, '0');
			}
			debug_putstring(buffer);
			break;
		case 'c':
			debug_putchar(__builtin_va_arg(varargs, wordsize_t) & 0xff);
			++argument_counter;
			break;
		case 's':
			str = __builtin_va_arg(varargs, char *);
			++argument_counter;
			if (output_width) {
				buffer = format_string(str, output_width);
				debug_putstring(buffer);
			} else {
				debug_putstring(str);
			}
			break;
		}
	}
}

#define		TEMPORARY_BUFFER_SIZE		64
static char temporary_buffer[TEMPORARY_BUFFER_SIZE] = {};

static char *format_string(const char *str, uint32_t width) {
	char *buffer = temporary_buffer;
	uint32_t len = strlen(str);

	if (width >= TEMPORARY_BUFFER_SIZE) {
		width = TEMPORARY_BUFFER_SIZE-1;
	}

	if (len >= width) {
		memcpy(buffer, str, width);
		buffer[width] = '\0';
	} else if (len < width) {
		uint32_t i;
		memcpy(buffer + (width-len), str, len+1);
		for (i = 0; i < width-len; i++) {
			buffer[i] = ' ';
		}
	}

	return buffer;
}

static char *format_decimal(uint64_t value) {
	char *buffer = temporary_buffer + TEMPORARY_BUFFER_SIZE - 1;

	*buffer = '\0';
	while (value) {
		--buffer;
		*buffer = '0' + modulo_u64_u64(value, (uint64_t)10);
		value = divide_u64_u64(value, (uint64_t)10);
	}

	if (*buffer == '\0') {
		*(--buffer) = '0';
	}

	return buffer;
}

static char *format_hexadecimal(uint64_t value) {
	char *buffer = temporary_buffer + TEMPORARY_BUFFER_SIZE - 1;

	*buffer = '\0';
	while (value) {
		--buffer;
		if ((value & 0xf) < 0xa) {
			*buffer = '0' + (value & 0xf);
		} else {
			*buffer = ('a' - 10) + (value & 0xf);
		}
		value >>= 4;
	}

	if (*buffer == '\0') {
		*(--buffer) = '0';
	}

	return buffer;
}

static char *pad_buffer(char *buffer, uint32_t width, uint8_t pad_char) {
	uint32_t length = strlen(buffer);

	if (width >= TEMPORARY_BUFFER_SIZE)
		width = TEMPORARY_BUFFER_SIZE - 1;

	if (length >= width)
		return buffer;

	while (length < width) {
		--buffer;
		*buffer = pad_char;
		++length;
	}

	return buffer;
}
