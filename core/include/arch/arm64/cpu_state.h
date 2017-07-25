struct vm_cpu_state_ {
	/* 0x000 */
	uint64_t	gpregs[32];	// EL1: x0-x31, x31 doubles as SP_EL1

	/* 0x100 */
	uint64_t	pstate;
	uint64_t	pc;
	uint64_t	syndrome;
	uint64_t	lightweight_flags;

	/* 0x120 */
	uint64_t	esr_el1;
	uint64_t	elr_el1;
	uint64_t	spsr_el1;
	uint64_t	cpacr_el1;

	/* 0x140 */
	uint64_t	sctlr_el1;
	uint64_t	dacr_el1;
	uint64_t	ttbcr_el1;
	uint64_t	ttbr0_el1;
	uint64_t	ttbr1_el1;
	uint64_t	far_el1;
	uint64_t	afsr0_el1;
	uint64_t	afsr1_el1;

	/* 0x180 */
	uint64_t	mair_el1;	// MAIR0..MAIR1 (or PRRR..NMRR)
	uint64_t	amair_el1;
	uint64_t	vbar_el1;

	/* 0x198 */
	uint64_t	contextidr_el1;
	uint64_t	tpidr_el1;	// prw
	uint64_t	tpidrro_el0;	// uro
	uint64_t	tpidr_el0;	// urw
	uint64_t	sp_el0;

	/* 0x1c0 */
	uint64_t	fpcr;
	uint64_t	fpsr;
	uint64_t	fpregs[64];

	/* 0x3d0 */
	uint64_t	mdccint_el1;
	uint64_t	mdscr_el1;

	/* 0x3e0 */
	uint64_t	osdlr_el1;
	uint64_t	oseccr_el1;
	uint64_t	osdtrrx_el1;
	uint64_t	osdtrtx_el1;

	/* 0x400 */
	uint64_t	cntkctl_el1;
};

typedef struct vm_cpu_state_	vm_cpu_state;
