#include <asm/assembler.h>
#include <asm/offsets.h>
#include <arch/asm/cp0_regs.h>
#include <arch/cpu_state.h>

.global	vm_cpu_state_save_full
.global	vm_cpu_state_load_full

vm_cpu_state_save_full:
	/* obtain pointer to cpu_state of current_vm_cpu */
	lw	$t0, current_vm_cpu
	lw	$t0, offset(vm_cpu, cpu_state)($t0)

	/* store EPC into state->pc */
	mfc0	$t1, c0_epc
	sw	$t1, offset(vm_cpu_state, pc)($t0)

	/* store guests srsctl and associated hcp0_guestctl3 */
	mfgc0	$t1, c0_srsctl
	sw	$t1, offset(vm_cpu_state, cp0_srsctl)($t0)
	mfc0	$t1, c0_guestctl3
	sw	$t1, offset(vm_cpu_state, hcp0_guestctl3)($t0)

	/* store guests status */
	mfgc0	$t1, c0_status
	sw	$t1, offset(vm_cpu_state, cp0_status)($t0)

	jr	$ra
ASM_FUNCTION_END vm_cpu_state_save_full

vm_cpu_state_load_full:
	/* set FCD, CGI in GeustCtl0Ext to not get EXCs on Field changes */
	mfc0	$t1, c0_guestctl0ext
	ori	$t1, 0x18
	mtc0	$t1, c0_guestctl0ext

	/* set guest ID */
	mfc0	$t1, c0_guestctl1
	lui	$t1, 1
	ori	$t1, 1
	mtc0	$t1, c0_guestctl1

	/* set GM, GT, CG in GuestCtl0 */
	mfc0	$t1, c0_guestctl0
	lui	$t2, 0xDF83
	ori	$t2, 0xF300
	or	$t1, $t1, $t2
	mtc0	$t1, c0_guestctl0

	/* obtain pointer to cpu_state of current_vm_cpu */
	lw	$t0, current_vm_cpu
	lw	$t0, offset(vm_cpu, cpu_state)($t0)

	/* load pc and put into EPC */
	lw	$t1, offset(vm_cpu_state, pc)($t0)
	mtc0	$t1, c0_epc

	/* load guests status*/
	lw	$t1, offset(vm_cpu_state, cp0_status)($t0)
	mtgc0	$t1, c0_status

	/* load srsctl and associated hosts guestctl3 */
	lw	$t1, offset(vm_cpu_state, cp0_srsctl)($t0)
	mtgc0	$t1, c0_srsctl
	lw	$t1, offset(vm_cpu_state, hcp0_guestctl3)($t0)
	mtc0	$t1, c0_guestctl3

	jr	$ra
ASM_FUNCTION_END vm_cpu_state_load_full
