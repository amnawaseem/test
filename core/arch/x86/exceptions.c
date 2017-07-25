#include <phidias.h>

void exception_handler(uint64_t *trap_state) {
	printf("Caught CPU Exception #%d, EC %d\r\n", trap_state[0], trap_state[1]);
	printf("CS:RIP %x:%x (FLAGS %x)\r\n", trap_state[3], trap_state[2], trap_state[4]);
	printf("SS:RSP %x:%x\r\n", trap_state[6], trap_state[5]);
}
