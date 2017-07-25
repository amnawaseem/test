#include <phidias.h>

void x86_virt_setup(void) {
	uint32_t efer_svm_value = 0x1000;
	void *hsave_pa;

	asm volatile("rdmsr\n\tor %0, %%eax\n\twrmsr" :: "r" (efer_svm_value), "d" (0), "c" (0xc0000080) : "eax");

	hsave_pa = (void *)(core_memarea(MEMAREA_VMCX)->paddr + 0x0000);

	asm volatile("wrmsr" :: "d" (0), "a" (hsave_pa), "c" (0xc0010117));
}
