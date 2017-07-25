#include <phidias.h>

uint32_t vmcs_revision = 0;

void x86_virt_setup(void) {
	uint32_t tmp, tmp_upper;
	uint32_t *hsave_va = (void *)(core_memarea(MEMAREA_VMCX)->vaddr + 0x0000);
	void *hsave_pa = (void *)(core_memarea(MEMAREA_VMCX)->paddr + 0x0000);

	// Step 1: enable CR4.VMXE
	asm volatile("mov %%cr4, %%rax\n\tor $0x2000, %%rax\n\tmov %%rax, %%cr4" ::: "rax");

	// Step 2: read IA32_VMX_BASIC
	asm volatile("rdmsr" : "=a" (vmcs_revision) : "c" (0x480) : "edx");

	// Step 3: initialize VMXON region
	hsave_va[0] = vmcs_revision;

#if 1
	// Step pre4: check IA32_FEATURE_CONTROL and the CR[04]_FIXED[01] MSRs
	asm volatile("rdmsr" : "=a" (tmp) : "c" (0x3a) : "edx");
	printf("FEATURE_CONTROL: %x\r\n", tmp);
	asm volatile("rdmsr" : "=a" (tmp) : "c" (0x486) : "edx");
	printf("CR0_0: %x\r\n", tmp);
	asm volatile("rdmsr" : "=a" (tmp) : "c" (0x487) : "edx");
	printf("CR0_1: %x\r\n", tmp);
	asm volatile("rdmsr" : "=a" (tmp) : "c" (0x488) : "edx");
	printf("CR4_0: %x\r\n", tmp);
	asm volatile("rdmsr" : "=a" (tmp) : "c" (0x489) : "edx");
	printf("CR4_1: %x\r\n", tmp);

	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x480));
	printf("BASIC: %x:%x\r\n", tmp_upper, tmp);
	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x481));
	printf("PIN: %x:%x\r\n", tmp_upper, tmp);
	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x482));
	printf("PROC: %x:%x\r\n", tmp_upper, tmp);
	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x483));
	printf("EXIT: %x:%x\r\n", tmp_upper, tmp);
	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x484));
	printf("ENTRY: %x:%x\r\n", tmp_upper, tmp);
	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x48b));
	printf("PROC2: %x:%x\r\n", tmp_upper, tmp);
	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x48d));
	printf("tPIN: %x:%x\r\n", tmp_upper, tmp);
	asm volatile("rdmsr" : "=a" (tmp), "=d" (tmp_upper) : "c" (0x48e));
	printf("tPROC: %x:%x\r\n", tmp_upper, tmp);
#endif

	// Step 4: VMXON
	asm volatile("vmxon %0" :: "m" (hsave_pa));
}
