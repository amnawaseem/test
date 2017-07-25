#include <phidias.h>
#include <drivers/timer.h>

uint32_t timer_interrupt() {
#if defined(TIMER_DRIVER_LAPIC)
	return timer_lapic_interrupt(core_memarea(MEMAREA_LAPIC));
#elif defined(TIMER_DRIVER_MPCORE)
	return timer_mpcore_interrupt(core_memarea(MEMAREA_MPCORE));
#elif defined(TIMER_DRIVER_ARM_GENERIC)
	return timer_arm_generic_interrupt();
#elif defined(TIMER_DRIVER_PIC32)
	/* on PIC32 this is not used but called directly by HW */
	return 0;
#else
# warning No timer driver selected!
#endif
}
