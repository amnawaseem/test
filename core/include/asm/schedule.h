#ifndef	_PHIDIAS_ASM_SCHEDULE_H_
#define	_PHIDIAS_ASM_SCHEDULE_H_

#define	SCHEDULER_CLASS_FAIR_SHARE	1
#define	SCHEDULER_CLASS_REALTIME	2

// this VCPU is in a runqueue or currently executing
#define	SCHEDULER_STATE_READY		1

// this VCPU has issued the architectural 'wait' instruction (hlt, wfi...)
// and will resume when an interrupt is posted to it
#define	SCHEDULER_STATE_WAITING		2

// this VCPU is a secondary core and waiting for the VM's primary core to
// issue the startup command
#define	SCHEDULER_STATE_DORMANT		3

// this VCPU has died due to illegal behaviour (out-of-bounds nested page
// fault etc.)
#define	SCHEDULER_STATE_ZOMBIE		4

#endif
