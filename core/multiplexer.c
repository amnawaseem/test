#include <phidias.h>
#include <multiplexer.h>
#include <drivers/uart.h>
#include <specification.h>
#include <vm.h>
#include <emulate/uart.h>

uint8_t __shared active_channel_in = 0;
uint8_t __shared active_channel_out = 0;
uint8_t __shared change_imminent = 0;
uint32_t __shared linemux_lock;


static inline void switch_channel(uint8_t id) {
	if(id != active_channel_out) {
		spinlock_lock(&linemux_lock);
		active_channel_out = id;
		spinlock_unlock(&linemux_lock);
		uart_putchar(0xff);
		uart_putchar(active_channel_out);

	}
}

static inline void mux_out_string_fast(const char* string) {
	switch_channel(cpu_number);
	uart_putstring(string);
}

static inline void mux_out_string_slow(const char* string) {
	switch_channel(cpu_number);
	while(*string) {
		if(((uint8_t)*string) == 0xff) 
			uart_putchar(0xff);
		uart_putchar(*string);
		++string;
	}
}

inline void mux_out_string(const char* string) {
	const char* S = string;

	while (*S) {
		if(((uint8_t)*S) == 0xff) {
			mux_out_string_slow(string);
			return;
		}
		++S;
	}
	mux_out_string_fast(string);

}

inline void mux_out_char(uint8_t val,uint32_t id) {
	switch_channel((id << 4) | cpu_number);

	/* If a guest wants to transmit "0xff" we need to escape it */
	if (val == 0xff)
		uart_putchar(0xff);
	uart_putchar(val);

}

inline void mux_in_char(uint8_t ch) {
	uint32_t i;
	if (change_imminent && (ch == 0xff)) {
		change_imminent = 0;
	} else if (ch == 0xff) {
		change_imminent = 1;
		return;
	} else if(change_imminent) {
		active_channel_in = ch;
		change_imminent = 0;
		return;
	}
	// Find the matching vm_cpu
	for (i = 0; i < _specification.cpus[cpu_number].num_vm_cpus;i++) {
		if(((_specification.cpus[cpu_number].vm_cpus[i]->pagetable_identifier << 4) | cpu_number) 
				== active_channel_in) {
			emulate_uart_push_character(_specification.cpus[cpu_number].vm_cpus[i],ch);
			//TODO: Return vcpu pointer to push char to
			return;
		}

	}
	//Char for phidias? What shall it do with it?
	if(active_channel_in < 8)
		return;
	if (i == _specification.cpus[cpu_number].num_vm_cpus) {
		// The state of the serial multiplexer is bogus...
		printf("ERROR, couldn't find a matching vm_cpu for id=0x%x\n",
				active_channel_in);
		panic();
	}

}
