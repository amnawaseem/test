#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <schedule.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>
#include <arch/cp15_regs.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <trace.h>

#define	WRITE_CPU_STATE(regname, reg)		{			\
	vm_cpu_cp->cpu_state->regname = reg;				\
}

#define	READ_CPU_STATE(regname, reg)		{			\
	reg = vm_cpu_cp->cpu_state->regname;				\
}

void coproc_write(const vm_cpu *vm_cpu_cp, uint32_t coproc, uint32_t code, uint32_t reg) {
	// printf("G %x:%x #CPW %x:%x\r\n", vm_cpu_cp->cpu_state->cpsr, vm_cpu_cp->cpu_state->pc, coproc, code);

	switch (code | (coproc << 16)) {
	case COPROC_CODE_TLBIALL:
	case COPROC_CODE_TLBIALLIS:
		emulate_vtlb_flush_current(vm_cpu_cp);
		COPROCESSOR_WRITE(COPROC_CODE_TLBIALL, 0);
		break;
	case COPROC_CODE_SCTLR:
		if ((reg & SCTLR_BIT_M) && !(vm_cpu_cp->cpu_state->sctlr & SCTLR_BIT_M)) {
			emulate_vtlb_switch(vm_cpu_cp, vm_cpu_cp->cpu_state->ttbr0, vm_cpu_cp->cpu_state->contextidr & 0xff);
		} else if (!(reg & SCTLR_BIT_M) && (vm_cpu_cp->cpu_state->sctlr & SCTLR_BIT_M)) {
			emulate_vtlb_switch(vm_cpu_cp, VTLB_INVALID_PAGETABLE, VTLB_INVALID_IDENTIFIER);
		}
		WRITE_CPU_STATE(sctlr, reg);
		break;
	case COPROC_CODE_CPACR:
		break;
	case COPROC_CODE_TTBR0:
		WRITE_CPU_STATE(ttbr0, reg);
		// printf("TTBR0 %x\r\n", reg);
		if (vm_cpu_cp->cpu_state->sctlr & SCTLR_BIT_M) {
			emulate_vtlb_switch(vm_cpu_cp, vm_cpu_cp->cpu_state->ttbr0, vm_cpu_cp->cpu_state->contextidr & 0xff);
		}
		break;
	case COPROC_CODE_TTBR1: WRITE_CPU_STATE(ttbr1, reg); break;
	case COPROC_CODE_TTBCR: WRITE_CPU_STATE(ttbcr, reg); break;
	case COPROC_CODE_DACR: WRITE_CPU_STATE(dacr, reg); break;
	case COPROC_CODE_CSSELR: break;
	case COPROC_CODE_BPIALLIS: COPROCESSOR_WRITE(COPROC_CODE_BPIALLIS, 0); break;
	case COPROC_CODE_BPIALL: COPROCESSOR_WRITE(COPROC_CODE_BPIALL, 0); break;
	case COPROC_CODE_ICIALLU: COPROCESSOR_WRITE(COPROC_CODE_ICIALLU, 0); break;
	case COPROC_CODE_ICIMVAU: COPROCESSOR_WRITE(COPROC_CODE_ICIMVAU, reg); break;
	// Way == Set == 0: replay DC*SW loop to hardware
	case COPROC_CODE_DCIMVAC: COPROCESSOR_WRITE(COPROC_CODE_DCIMVAC, reg); break;
	case COPROC_CODE_DCISW:
		if ((reg >> 4) == 0)
			cpu_dcache_flush(0, 1);
		break;
	case COPROC_CODE_DCCMVAC: COPROCESSOR_WRITE(COPROC_CODE_DCCMVAC, reg); break;
	case COPROC_CODE_DCCSW:
		if ((reg >> 4) == 0)
			cpu_dcache_flush(1, 0);
		break;
	case COPROC_CODE_DCCMVAU: COPROCESSOR_WRITE(COPROC_CODE_DCCMVAU, reg); break;
	case COPROC_CODE_DCCIMVAC: COPROCESSOR_WRITE(COPROC_CODE_DCCIMVAC, reg); break;
	case COPROC_CODE_DCCISW:
		if ((reg >> 4) == 0)
			cpu_dcache_flush(1, 1);
		break;
	case COPROC_CODE_TLBIMVA:
		emulate_vtlb_unmap_by_identifier(current_vm_cpu, reg & 0xfff, reg & ~0xfff);
		break;
	case COPROC_CODE_TLBIASID: emulate_vtlb_flush_by_identifier(current_vm_cpu, reg); break;
	case COPROC_CODE_PRRR: WRITE_CPU_STATE(prrr, reg); break;
	case COPROC_CODE_NMRR: WRITE_CPU_STATE(nmrr, reg); break;
	case COPROC_CODE_CONTEXTIDR:
		WRITE_CPU_STATE(contextidr, reg);
		// printf("CTXTIDR %x\r\n", reg);
		if (vm_cpu_cp->cpu_state->sctlr & SCTLR_BIT_M) {
			emulate_vtlb_switch(vm_cpu_cp, vm_cpu_cp->cpu_state->ttbr0, vm_cpu_cp->cpu_state->contextidr & 0xff);
		}
		break;
	case COPROC_CODE_TPIDRURO: WRITE_CPU_STATE(tpidruro, reg); COPROCESSOR_WRITE(COPROC_CODE_TPIDRURO, reg); break;
	case COPROC_CODE_TPIDRPRW: WRITE_CPU_STATE(tpidrprw, reg); break;

	case COPROC_CODE_FPEXC: COPROCESSOR_WRITE(COPROC_CODE_FPEXC, reg); break;
	default:
		printf("G %x:%x #CPW %x:%x\r\n",
			vm_cpu_cp->cpu_state->cpsr, vm_cpu_cp->cpu_state->pc, coproc, code);
		printf("Unsupported cp15 code W:%4x\r\n", code);
		panic();
	}
}

uint32_t coproc_read(const vm_cpu *vm_cpu_cp, uint32_t coproc, uint32_t code) {
	uint32_t reg;

	switch (code | (coproc << 16)) {
	case COPROC_CODE_MIDR: COPROCESSOR_READ(COPROC_CODE_MIDR, reg); break;
	case COPROC_CODE_CTR: COPROCESSOR_READ(COPROC_CODE_CTR, reg); break;
	case COPROC_CODE_MPIDR: READ_CPU_STATE(mpidr, reg); break;
	case COPROC_CODE_ID_PFR0: COPROCESSOR_READ(COPROC_CODE_ID_PFR0, reg); reg &= ~0x0000f000; break;
	case COPROC_CODE_ID_PFR1: COPROCESSOR_READ(COPROC_CODE_ID_PFR1, reg); break;
	case COPROC_CODE_ID_PFR2: COPROCESSOR_READ(COPROC_CODE_ID_PFR2, reg); break;
	case COPROC_CODE_ID_PFR3: COPROCESSOR_READ(COPROC_CODE_ID_PFR3, reg); break;
	case COPROC_CODE_ID_MMFR0: COPROCESSOR_READ(COPROC_CODE_ID_MMFR0, reg); break;
	case COPROC_CODE_ID_MMFR1: COPROCESSOR_READ(COPROC_CODE_ID_MMFR1, reg); break;
	case COPROC_CODE_ID_MMFR2: COPROCESSOR_READ(COPROC_CODE_ID_MMFR2, reg); break;
	case COPROC_CODE_ID_MMFR3: COPROCESSOR_READ(COPROC_CODE_ID_MMFR3, reg); break;
	case COPROC_CODE_ID_ISAR0: COPROCESSOR_READ(COPROC_CODE_ID_ISAR0, reg); break;
	case COPROC_CODE_ID_ISAR1: COPROCESSOR_READ(COPROC_CODE_ID_ISAR1, reg); break;
	case COPROC_CODE_ID_ISAR2: COPROCESSOR_READ(COPROC_CODE_ID_ISAR2, reg); break;
	case COPROC_CODE_ID_ISAR3: COPROCESSOR_READ(COPROC_CODE_ID_ISAR3, reg); break;
	case COPROC_CODE_ID_ISAR4: COPROCESSOR_READ(COPROC_CODE_ID_ISAR4, reg); break;
	case COPROC_CODE_ID_ISAR5: COPROCESSOR_READ(COPROC_CODE_ID_ISAR5, reg); break;
	case COPROC_CODE_SCTLR: READ_CPU_STATE(sctlr, reg); break;
	case COPROC_CODE_CPACR: reg = 0xdfffffff; break;
	case COPROC_CODE_TTBR0: READ_CPU_STATE(ttbr0, reg); break;
	case COPROC_CODE_TTBR1: READ_CPU_STATE(ttbr1, reg); break;
	case COPROC_CODE_TTBCR: READ_CPU_STATE(ttbcr, reg); break;
	case COPROC_CODE_DACR: READ_CPU_STATE(dacr, reg); break;
	case COPROC_CODE_DFSR: READ_CPU_STATE(dfsr, reg); break;
	case COPROC_CODE_IFSR: READ_CPU_STATE(ifsr, reg); break;
	case COPROC_CODE_DFAR: READ_CPU_STATE(dfar, reg); break;
	case COPROC_CODE_IFAR: READ_CPU_STATE(ifar, reg); break;
	case COPROC_CODE_PRRR: READ_CPU_STATE(prrr, reg); break;
	case COPROC_CODE_NMRR: READ_CPU_STATE(nmrr, reg); break;
	case COPROC_CODE_CONTEXTIDR: READ_CPU_STATE(contextidr, reg); break;
	case COPROC_CODE_TPIDRPRW: READ_CPU_STATE(tpidrprw, reg); break;
	case COPROC_CODE_CCSIDR: COPROCESSOR_READ(COPROC_CODE_CCSIDR, reg); break;
	case COPROC_CODE_CLIDR: COPROCESSOR_READ(COPROC_CODE_CLIDR, reg); break;
	case COPROC_SPECIFIC_CODE_MPCORE_CBAR: reg = 0x1e000000; break;

	case COPROC_CODE_FPSID: COPROCESSOR_READ(COPROC_CODE_FPSID, reg); break;
	case COPROC_CODE_MVFR1: COPROCESSOR_READ(COPROC_CODE_MVFR1, reg); break;
	case COPROC_CODE_MVFR0: COPROCESSOR_READ(COPROC_CODE_MVFR0, reg); break;
	case COPROC_CODE_FPEXC: COPROCESSOR_READ(COPROC_CODE_FPEXC, reg); break;
	default:
		printf("G %x:%x #CPR %x:%x\r\n",
			vm_cpu_cp->cpu_state->cpsr, vm_cpu_cp->cpu_state->pc, coproc, code);
		printf("Unsupported cp15 code R:%4x\r\n", code);
		panic();
	}

	return reg;
}
