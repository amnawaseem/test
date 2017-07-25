#ifndef	__ASSEMBLER__

struct vm_cpu_state_ {
	/* list all regs */
	uint32_t 	pc;
	uint32_t	cp0_srsctl;
	uint32_t	hcp0_guestctl3;
	uint32_t	cp0_status;
};
typedef struct vm_cpu_state_	vm_cpu_state;
#else
# define ASSEMBLER
# include <asm/offsets.h>
# include <arch/asm/cp0_regs.h>
	/* Save all registers that are in immediate danger */
	.macro	vm_cpu_state_save
	.set noat
	.set nomicromips
	.set noreorder

	/* restore sp and gp */
	lw	$sp, top_of_stack

	/* set the correct PSS (=GLSS + Guest.CSS), HW does not do that */
	mfc0	$t0, c0_guestctl3
	mfgc0	$t1, c0_srsctl
	andi	$t1, $t1, 0xF
	add	$t1, $t1, $t0
	mfc0	$t0, c0_srsctl
	ins	$t0, $t1, 6, 4
	mtc0	$t0, c0_srsctl
	.endm
#endif
