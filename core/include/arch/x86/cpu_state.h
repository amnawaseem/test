#ifndef	__ASSEMBLER__

struct vm_cpu_state_vmcb_;

struct vm_cpu_state_ {
	struct vm_cpu_state_vmcb_	*vmcb;
	uintptr_t			vmcb_pa;

	uint64_t			rax;	/* unused on SVM */
	uint64_t			rbx;
	uint64_t			rcx;
	uint64_t			rdx;
	uint64_t			rsi;
	uint64_t			rdi;
	uint64_t			rbp;
	uint64_t			r8;
	uint64_t			r9;
	uint64_t			r10;
	uint64_t			r11;
	uint64_t			r12;
	uint64_t			r13;
	uint64_t			r14;
	uint64_t			r15;

	uint64_t			guest_cr0;
	uint64_t			guest_cr3;
	uint64_t			guest_cr4;
};
typedef struct vm_cpu_state_	vm_cpu_state;

struct vm_cpu_state_vmcb_ {
	/* Control Area */

	// 0x00
	uint32_t	intercept_cr;
	uint32_t	intercept_dr;
	uint32_t	intercept_exceptions;
	uint32_t	intercept_insn0;

	// 0x10
	uint32_t	intercept_insn1;
	uint8_t		__rsvd0[0x3c - 0x14];
	uint16_t	pause_filter_threshold;
	uint16_t	pause_filter_count;

	// 0x40
	uint64_t	iopm_base_pa;
	uint64_t	msrpm_base_pa;

	// 0x50
	uint64_t	tsc_offset;
	uint32_t	asid;
	uint32_t	tlb_control;

	// 0x60
	uint32_t	irq_tpr_control;
	uint32_t	v_intr_vector;
	uint64_t	irq_shadow;

	// 0x70
	uint64_t	exitcode;
	uint64_t	exitinfo1;

	// 0x80
	uint64_t	exitinfo2;
	uint64_t	exitintinfo;

	// 0x90
	uint64_t	np_sev_enable;
	uint64_t	avic_bar;

	// 0xa0
	uint8_t		__rsvd1[0xa8 - 0xa0];
	uint64_t	eventinj;

	// 0xb0
	uint64_t	nested_cr3;
	uint64_t	lbr_nestedvm_enable;

	// 0xc0
	uint64_t	vmcb_clean_bits;
	uint64_t	next_rip;

	// 0xd0
	uint8_t		n_bytes_fetched;
	uint8_t		bytes_fetched[15];

	// 0xe0
	uint64_t	avic_backing_page;
	uint8_t		__rsvd2a[8];

	// 0xf0
	uint64_t	avic_logical_table;
	uint64_t	avic_physical_table;
	uint8_t		__rsvd2b[0x400 - 0x100];

	/* State-Save Area */
	// 0x400
	uint16_t	es_sel;
	uint16_t	es_attrib;
	uint32_t	es_limit;
	uint64_t	es_base;

	// 0x410
	uint16_t	cs_sel;
	uint16_t	cs_attrib;
	uint32_t	cs_limit;
	uint64_t	cs_base;

	// 0x420
	uint16_t	ss_sel;
	uint16_t	ss_attrib;
	uint32_t	ss_limit;
	uint64_t	ss_base;

	// 0x430
	uint16_t	ds_sel;
	uint16_t	ds_attrib;
	uint32_t	ds_limit;
	uint64_t	ds_base;

	// 0x440
	uint16_t	fs_sel;
	uint16_t	fs_attrib;
	uint32_t	fs_limit;
	uint64_t	fs_base;

	// 0x450
	uint16_t	gs_sel;
	uint16_t	gs_attrib;
	uint32_t	gs_limit;
	uint64_t	gs_base;

	// 0x460
	uint16_t	gdtr_sel;
	uint16_t	gdtr_attrib;
	uint32_t	gdtr_limit;
	uint64_t	gdtr_base;

	// 0x470
	uint16_t	ldtr_sel;
	uint16_t	ldtr_attrib;
	uint32_t	ldtr_limit;
	uint64_t	ldtr_base;

	// 0x480
	uint16_t	idtr_sel;
	uint16_t	idtr_attrib;
	uint32_t	idtr_limit;
	uint64_t	idtr_base;

	// 0x490
	uint16_t	tr_sel;
	uint16_t	tr_attrib;
	uint32_t	tr_limit;
	uint64_t	tr_base;

	// 0x4a0
	uint8_t		__rsvd3[0x4cb - 0x4a0];
	uint8_t		cpl;
	uint32_t	__rsvd4;

	// 0x4d0
	uint64_t	efer;
	uint8_t		__rsvd5[0x548 - 0x4d8];
	uint64_t	cr4;

	// 0x550
	uint64_t	cr3;
	uint64_t	cr0;

	// 0x560
	uint64_t	dr7;
	uint64_t	dr6;

	// 0x570
	uint64_t	rflags;
	uint64_t	rip;

	// 0x580
	uint8_t		__rsvd6[0x5d8 - 0x580];
	uint64_t	rsp;

	// 0x5e0
	uint8_t		__rsvd7[0x5f8 - 0x5e0];
	uint64_t	rax;

	// 0x600
	uint64_t	star;
	uint64_t	lstar;

	// 0x610
	uint64_t	cstar;
	uint64_t	sfmask;

	// 0x620
	uint64_t	kernelgsbase;
	uint64_t	sysenter_cs;

	// 0x630
	uint64_t	sysenter_esp;
	uint64_t	sysenter_eip;

	// 0x640
	uint64_t	cr2;
	uint8_t		__rsvd8[0x668 - 0x648];
	uint64_t	g_pat;

	// 0x670
	uint64_t	dbgctl;
	uint64_t	br_from;

	// 0x680
	uint64_t	br_to;
	uint64_t	lastexcp_from;

	// 0x690
	uint64_t	lastexcp_to;
};
typedef struct vm_cpu_state_vmcb_	vm_cpu_state_vmcb;

_Static_assert(sizeof(vm_cpu_state_vmcb) == 0x698, "VMCB Structure");

#endif
