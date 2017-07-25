#include <phidias.h>
#include <bootup.h>
#include <mmio.h>
#include <memarea.h>

void platform_boot_cpus() {
	const memarea *mailbox_area = core_memarea(MEMAREA_MBOX);
	uintptr_t bootup_function = core_physical_address((uintptr_t)&__boot);

	/* Write boot pointer into MAILBOX3 of CORE{1,2,3} */
	mmio_write32(mailbox_area->vaddr + 0x08c + 0x010, bootup_function);
	mmio_write32(mailbox_area->vaddr + 0x08c + 0x020, bootup_function);
	mmio_write32(mailbox_area->vaddr + 0x08c + 0x030, bootup_function);

	/* Note: contrary to the Linux SMP bootup code the other cores spin
	   instead of blocking in a WFI loop; so we don't have to do
	   anything else */
}
