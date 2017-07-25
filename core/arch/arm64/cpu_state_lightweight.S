#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/sysregs.h>

.global	vm_cpu_state_save_full
.global	vm_cpu_state_load_full

.pushsection	.data, "aw"

lightweight_owner_el1:
	.quad	0
lightweight_owner_fpu:
	.quad	0

.popsection

/* Note that SP_EL1 is already saved and restored in the 'light' switch part,
 * so we always have a consistent view of the guest's gpregs[].
 */
vm_cpu_state_save_el1:
	mrs	x2, ESR_EL1
	mrs	x3, ELR_EL1
	mrs	x4, SPSR_EL1
	mrs	x5, CPACR_EL1
	stnp	x2, x3, [x1, #offset(vm_cpu_state, esr_el1)]
	stnp	x4, x5, [x1, #offset(vm_cpu_state, spsr_el1)]

	mrs	x2, SCTLR_EL1
	mrs	x3, DACR32_EL2
	mrs	x4, TCR_EL1
	mrs	x5, TTBR0_EL1
	mrs	x6, TTBR1_EL1
	mrs	x7, FAR_EL1
	stnp	x2, x3, [x1, #offset(vm_cpu_state, sctlr_el1)]
	stnp	x4, x5, [x1, #offset(vm_cpu_state, ttbcr_el1)]
	stnp	x6, x7, [x1, #offset(vm_cpu_state, ttbr1_el1)]

	mrs	x2, AFSR0_EL1
	mrs	x3, AFSR1_EL1
	mrs	x4, MAIR_EL1
	mrs	x5, AMAIR_EL1
	mrs	x6, VBAR_EL1
	mrs	x7, CONTEXTIDR_EL1
	stnp	x2, x3, [x1, #offset(vm_cpu_state, afsr0_el1)]
	stnp	x4, x5, [x1, #offset(vm_cpu_state, mair_el1)]
	stnp	x6, x7, [x1, #offset(vm_cpu_state, vbar_el1)]
	mrs	x2, TPIDR_EL1
	str	x2, [x1, #offset(vm_cpu_state, tpidr_el1)]

	add	x1, x1, #0x400

	mrs	x2, MDCCINT_EL1
	mrs	x3, MDSCR_EL1
	mrs	x4, OSDLR_EL1
	mrs	x5, OSECCR_EL1
	mrs	x6, OSDTRRX_EL1
	mrs	x7, OSDTRTX_EL1
	stnp	x2, x3, [x1, #offset(vm_cpu_state, mdccint_el1) - 0x400]
	stnp	x4, x5, [x1, #offset(vm_cpu_state, osdlr_el1) - 0x400]
	stnp	x6, x7, [x1, #offset(vm_cpu_state, osdtrrx_el1) - 0x400]

	ret

vm_cpu_state_save_fpu:
	mrs	x2, FPCR
	mrs	x3, FPSR
	stp	x2, x3, [x1, #offset(vm_cpu_state, fpcr)]
	stnp	q0, q1, [x1, #offset(vm_cpu_state, fpregs) +   0]
	stnp	q2, q3, [x1, #offset(vm_cpu_state, fpregs) +  32]
	stnp	q4, q5, [x1, #offset(vm_cpu_state, fpregs) +  64]
	stnp	q6, q7, [x1, #offset(vm_cpu_state, fpregs) +  96]
	stnp	q8, q9, [x1, #offset(vm_cpu_state, fpregs) + 128]
	stnp	q10, q11, [x1, #offset(vm_cpu_state, fpregs) + 160]
	stnp	q12, q13, [x1, #offset(vm_cpu_state, fpregs) + 192]
	stnp	q14, q15, [x1, #offset(vm_cpu_state, fpregs) + 224]
	stnp	q16, q17, [x1, #offset(vm_cpu_state, fpregs) + 256]
	stnp	q18, q19, [x1, #offset(vm_cpu_state, fpregs) + 288]
	stnp	q20, q21, [x1, #offset(vm_cpu_state, fpregs) + 320]
	stnp	q22, q23, [x1, #offset(vm_cpu_state, fpregs) + 352]
	stnp	q24, q25, [x1, #offset(vm_cpu_state, fpregs) + 384]
	stnp	q26, q27, [x1, #offset(vm_cpu_state, fpregs) + 416]
	stnp	q28, q29, [x1, #offset(vm_cpu_state, fpregs) + 448]
	stnp	q30, q31, [x1, #offset(vm_cpu_state, fpregs) + 480]
	ret

/**
 * Schedule out the VCPU in x0.
 * Don't save anything in particular now - load_full will take care of that.
 */
vm_cpu_state_save_full:
	ldr	x1, [x0, #offset(vm_cpu, cpu_state)]
	mrs	x2, TPIDRRO_EL0
	mrs	x3, TPIDR_EL0
	mrs	x4, SP_EL0
	mrs	x5, CNTKCTL_EL1
	stnp	x2, x3, [x1, #offset(vm_cpu_state, tpidrro_el0)]
	str	x4, [x1, #offset(vm_cpu_state, sp_el0)]
	str	x5, [x1, #offset(vm_cpu_state, cntkctl_el1)]
	ret

/***************************************************************************/

vm_cpu_state_load_el1:
	ldnp	x2, x3, [x1, #offset(vm_cpu_state, esr_el1)]
	ldnp	x4, x5, [x1, #offset(vm_cpu_state, spsr_el1)]
	msr	ESR_EL1, x2
	msr	ELR_EL1, x3
	msr	SPSR_EL1, x4
	msr	CPACR_EL1, x5

	ldnp	x2, x3, [x1, #offset(vm_cpu_state, sctlr_el1)]
	ldnp	x4, x5, [x1, #offset(vm_cpu_state, ttbcr_el1)]
	ldnp	x6, x7, [x1, #offset(vm_cpu_state, ttbr1_el1)]
	msr	SCTLR_EL1, x2
	msr	DACR32_EL2, x3
	msr	TCR_EL1, x4
	msr	TTBR0_EL1, x5
	msr	TTBR1_EL1, x6
	msr	FAR_EL1, x7

	ldnp	x2, x3, [x1, #offset(vm_cpu_state, afsr0_el1)]
	ldnp	x4, x5, [x1, #offset(vm_cpu_state, mair_el1)]
	ldnp	x6, x7, [x1, #offset(vm_cpu_state, vbar_el1)]
	msr	AFSR0_EL1, x2
	msr	AFSR1_EL1, x3
	msr	MAIR_EL1, x4
	msr	AMAIR_EL1, x5
	msr	VBAR_EL1, x6
	msr	CONTEXTIDR_EL1, x7
	ldr	x2, [x1, #offset(vm_cpu_state, tpidr_el1)]
	msr	TPIDR_EL1, x2

	add	x1, x1, #0x400

	ldnp	x2, x3, [x1, #offset(vm_cpu_state, mdccint_el1) - 0x400]
	ldnp	x4, x5, [x1, #offset(vm_cpu_state, osdlr_el1) - 0x400]
	ldnp	x6, x7, [x1, #offset(vm_cpu_state, osdtrrx_el1) - 0x400]
	msr	MDCCINT_EL1, x2
	msr	MDSCR_EL1, x3
	msr	OSDLR_EL1, x4
	msr	OSECCR_EL1, x5
	msr	OSDTRRX_EL1, x6
	msr	OSDTRTX_EL1, x7

	ret

/*
 * Schedule in the VCPU in x0. For each resource set that x0 wants:
 *  - if the VCPU is already the owner, do nothing
 *  - if some other VCPU is owning it, make it relinquish the resource
 *    (so that it's unowned afterwards)
 *  - if nobody owns it (now), load our state
 */
vm_cpu_state_load_full:
	str	x30, [x29, #-48]!
	ldr	x1, [x0, #offset(vm_cpu, cpu_state)]
	stp	x0, x1, [x29, #16]
	stp	x27, x28, [x29, #32]

	ldr	x2, [x0, #offset(vm_cpu, pagetable_base)]
	ldr	x3, [x0, #offset(vm_cpu, pagetable_identifier)]
	and	x3, x3, #0xffff
	orr	x2, x2, x3, lsl #48
	msr	VTTBR_EL2, x2

	ldnp	x2, x3, [x1, #offset(vm_cpu_state, tpidrro_el0)]
	ldr	x4, [x1, #offset(vm_cpu_state, sp_el0)]
	msr	TPIDRRO_EL0, x2
	msr	TPIDR_EL0, x3
	msr	SP_EL0, x4

	ldr	x28, [x1, #offset(vm_cpu_state, lightweight_flags)]
	tbnz	x28, #4, .Lel1_undesired
	/* x0 wants EL1; check who owns it (and load CNTKCTL) */
	ldr	x3, [x1, #offset(vm_cpu_state, cntkctl_el1)]
	msr	CNTKCTL_EL1, x3
	ldr	x27, =lightweight_owner_el1
	ldr	x3, [x27]
	cbz	x3, .Lel1_unowned
	cmp	x0, x3
	beq	.Le1_returning
	mov	x0, x3
	ldr	x1, [x0, #offset(vm_cpu, cpu_state)]
	bl	vm_cpu_state_save_el1
	ldp	x0, x1, [x29, #16]
.Lel1_unowned:
	str	x0, [x27]
	bl	vm_cpu_state_load_el1
.Le1_returning:
	ldr	x2, .Lhcr_el1enabled
	msr	HCR_EL2, x2
	ldp	x27, x28, [x29, #32]
	ldr	x30, [x29], #48
	ret

.Lel1_undesired:
	ldr	x2, .Lhcr_el1disabled
	msr	HCR_EL2, x2
	mov	x2, #0x102
	msr	CNTKCTL_EL1, x2

	ldp	x27, x28, [x29, #32]
	ldr	x30, [x29], #48
	ret
.Lhcr_el1enabled:
	.quad   HCR_EL2_RW | HCR_EL2_TSC | HCR_EL2_TWE | HCR_EL2_TWI | HCR_EL2_AMO | HCR_EL2_IMO | HCR_EL2_FMO | HCR_EL2_VM
.Lhcr_el1disabled:
	.quad   HCR_EL2_RW | HCR_EL2_TGE | HCR_EL2_TSC | HCR_EL2_TWE | HCR_EL2_TWI | HCR_EL2_DC | HCR_EL2_AMO | HCR_EL2_IMO | HCR_EL2_FMO | HCR_EL2_VM
	.ltorg

/***************************************************************************/
