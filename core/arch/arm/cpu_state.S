#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/cpu_ops.h>
#include <arch/asm/cp15_regs.h>
#include <arch/cpu_state.h>

.global	vm_cpu_state_save_full
.global	vm_cpu_state_load_full
.global	emulate_vtlb_ensure_host_paging
.global	emulate_vtlb_ensure_guest_paging
.global	hw_contextidr

vm_cpu_state_save_full:
	push	{r4, lr}

	ldr	r1, [r0, #offset(vm_cpu, cpu_state)]

	mov	r0, #FPEXC_EN
	vmrs	r2, fpexc
	str	r2, [r1, #offset(vm_cpu_state, fpexc)]
	vmsr	fpexc, r0

	vmrs	r3, fpscr
	str	r3, [r1, #offset(vm_cpu_state, fpscr)]

	add	r4, r1, #offset(vm_cpu_state, fpregs)
	vstmia	r4!, {d0-d15}

	pop	{r4, lr}
ASM_FUNCTION_END vm_cpu_state_save_full

vm_cpu_state_load_full:
	push	{r4, lr}

	mov	r4, r0
	bl	emulate_vtlb_ensure_guest_paging

	ldr	r0, [r4, #offset(vm_cpu, cpu_state)]

	ldr	r2, [r0, #offset(vm_cpu_state, tpidruro)]
	ldr	r3, [r0, #offset(vm_cpu_state, tpidrurw)]
	mcr	TPIDRURO(r2)
	mcr	TPIDRURW(r3)

	add	r4, r0, #offset(vm_cpu_state, fpregs)
	vldmia	r4!, {d0-d15}

	ldr	r3, [r0, #offset(vm_cpu_state, fpscr)]
	vmsr	fpscr, r3

	ldr	r2, [r0, #offset(vm_cpu_state, fpexc)]
	vmsr	fpexc, r2

	pop	{r4, pc}
	.ltorg
ASM_FUNCTION_END vm_cpu_state_load_full

.pushsection .data, "aw"

hw_contextidr:
	.long	0

.popsection

/**
 * Parameters:
 *   r0    vm_cpu *
 *
 * Called to switch from VTLB guest to host paging.
 * Do nothing if host paging is already in effect.
 */
emulate_vtlb_ensure_host_paging:
	// if we are already in host paging mode, do nothing
	ldr	r3, =hw_contextidr
	ldr	r3, [r3]
	cmp	r3, #CONTEXTIDR_ASID_HOST_PAGING
	bxeq	lr

	ldr	r0, =_specification
	ldr	r1, =cpu_number
	ldr	r1, [r1]
	ldr	r0, [r0, #offset(specification, cpus)]
	add	r0, r0, r1, lsl #logsizeof(specification_cpu)
	ldr	r1, [r0, #offset(specification_cpu, pagetable_address)]
	mov	r0, #CONTEXTIDR_ASID_HOST_PAGING

	b	set_pagetable
	.ltorg
ASM_FUNCTION_END emulate_vtlb_ensure_host_paging

/**
 * Parameters:
 *   r0    vm_cpu *
 *
 * Called to switch back from VTLB host to guest paging, or when the VTLB has
 * selected a different instance.
 *
 * If the specified VM is not running, do nothing.
 * If it is, check whether the instance identifier matches the currently
 * active one.
 * If that is not the case, load the new pagetable and identifier.
 */
emulate_vtlb_ensure_guest_paging:
	// if we are messing with a non-running VTLB, do nothing
	ldr	r1, =current_vm_cpu
	ldr	r1, [r1]
	cmp	r1, r0
	bxne	lr

	// if this guest doesn't have a VTLB, there's no guest paging
	ldr	r1, [r0, #offset(vm_cpu, vtlb_emulate)]
	tst	r1, r1
	bxeq	lr

	// grab the active VTLB instance
	ldr	r2, [r1, #offset(emulate, control)]
	ldr	r3, [r2, #offset(emulate_vtlb, active_instance)]
	ldr	r1, [r2, #offset(emulate_vtlb, instances)]
	add	r1, r1, r3, lsl #logsizeof(emulate_vtlb_instance)

	// get identifier and pagetable address from VTLB instance
	ldr	r0, [r1, #offset(emulate_vtlb_instance, identifier)]
	ldr	r1, [r1, #offset(emulate_vtlb_instance, pagetable_address)]

	b	set_pagetable
	.ltorg
ASM_FUNCTION_END emulate_vtlb_ensure_guest_paging

/**
 * Parameters:
 *   r0    pagetable identifier (CONTEXTIDR)
 *   r1    pagetable address
 *
 * Loads r1 as new TTBR if r0 != hw_contextidr.
 */
set_pagetable:
	ldr	r2, =hw_contextidr
	ldr	r2, [r2]
	cmp	r0, r2
	bxeq	lr

	mov	r3, #CONTEXTIDR_ASID_RESERVED
	orr	r1, r1, #TTBR0_BIT_S | TTBR0_BIT_INNER_C_WBWA | TTBR0_BIT_OUTER_C_WBWA
	mcr	CONTEXTIDR(r3)
	isb
	ldr	r3, =hw_contextidr
	mcr	TTBR0(r1)
	isb
	mcr	CONTEXTIDR(r0)
	str	r0, [r3]
	bx	lr
	.ltorg
ASM_FUNCTION_END set_pagetable
