#include <phidias.h>
#include <schedule.h>
#include <vm.h>
#include <interrupts.h>
#include <misc_ops.h>
#include <arch/cpu_state.h>
#include <arch/cpu_ops.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>
#include <arch/emulate.h>
#include <arch/vm.h>
#include <arch/virt-vmx.h>
#include <arch/asm/vmx_regs.h>
#include <bootup.h>
#include <arch/specification.h>

#include <asm/lapic.h>
#include <mmio.h>

extern uintptr_t walker_x86_long(const struct vm_cpu_ *vm_cpu_walk,
		uintptr_t ptbl_base, uintptr_t address,
                uintptr_t *base, uintptr_t *size, uint32_t *flags);

void vm_cpu_dump_state() {
	vm_cpu_state *cpu_state = current_vm_cpu->cpu_state;
	uint64_t cr2v;

	asm volatile("mov %%cr2, %0" : "=r" (cr2v));

	printf("RAX/RBX/RCX/RDX: %x %x %x %x\r\n", cpu_state->rax, cpu_state->rbx,
				cpu_state->rcx, cpu_state->rdx);
	printf("RSI/RDI/RBP/RSP: %x %x %x %x\r\n", cpu_state->rsi, cpu_state->rdi,
				cpu_state->rbp, vmread(VMXREG_NWG_RSP));
	printf("R8-11          : %x %x %x %x\r\n", cpu_state->r8, cpu_state->r9,
				cpu_state->r10, cpu_state->r11);
	printf("R12-15         : %x %x %x %x\r\n", cpu_state->r12, cpu_state->r13,
				cpu_state->r14, cpu_state->r15);
	printf("RIP/RFLAGS: %x (%x)\r\n", vmread(VMXREG_NWG_RIP), vmread(VMXREG_NWG_RFLAGS));
	printf("CS/SS/TR: %x %x %x\r\n",
		vmread(VMXREG_16G_CS_SEL), vmread(VMXREG_16G_SS_SEL), vmread(VMXREG_32G_TR_ACCESS));
	printf("GDT/IDT: %x %x\r\n", vmread(VMXREG_NWG_GDTR_BASE), vmread(VMXREG_NWG_IDTR_BASE));
	printf("INSN ERROR? %x\r\n", vmread(VMXREG_32E_INSN_ERROR));
	printf("INSN LEN/INFO? %x/%x\r\n", vmread(VMXREG_32E_INSN_LENGTH), vmread(VMXREG_32E_INSN_INFO));
	printf("INTINFO / EC %x/%x\r\n", vmread(VMXREG_32E_EXIT_INTINFO), vmread(VMXREG_32E_EXIT_INT_EC));
	printf("VECTORING EC %x/%x\r\n", vmread(VMXREG_32E_IDT_VECINFO), vmread(VMXREG_32E_IDT_VEC_EC));
	printf("CR2: %x\r\n", cr2v);
}

void vm_cpu_dump_stack(const vm_cpu *stk_cpu) {
	uint64_t base, size;
	uint32_t flags, i;
	uint64_t *gpstk, *vstk;
	const memarea *stk_area;

	base = size = 0;
	printf("Stack Dump: ");
	gpstk = (uint64_t *)walker_x86_long(stk_cpu,
				vmread(VMXREG_NWG_CR3), vmread(VMXREG_NWG_RSP),
				&base, &size, &flags);

	if (size == 0) {
		printf("not available\r\n");
		return;
	} else
		printf("\r\n");

	stk_area = find_memarea_by_va(stk_cpu->memarea_tree, (uintptr_t)gpstk);
	if (stk_area == NULL) {
		printf("No memarea for gpRSP %x?\r\n", gpstk); return; }
	vstk = (void *)gpstk + stk_area->reference_area->vaddr - stk_area->vaddr;

	printf("RSP: %x -> %x -> %x\r\n", vmread(VMXREG_NWG_RSP), (uintptr_t)gpstk, (uintptr_t)vstk);
	for (i = 0; i < 16; i++) {
		printf("STK: %x %x %x %x\r\n", vstk[4*i], vstk[4*i+1], vstk[4*i+2], vstk[4*i+3]);
	}
}

void vm_cpu_initialize(const struct vm_cpu_ *new_vm_cpu) {
	vm_cpu_state *cpu_state = new_vm_cpu->cpu_state;
	const memarea *arch_memarea = find_memarea_by_va(_specification.cpus[cpu_number].memarea_tree, (uintptr_t)cpu_state);

	memset(cpu_state, 0, sizeof(vm_cpu_state));

	cpu_state->vmcb = ((void *)cpu_state) + 0x1000;
	cpu_state->vmcb_pa = arch_memarea->paddr + 0x1000;

	memset(cpu_state->vmcb, 0, sizeof(vm_cpu_state_vmcb));
	((uint32_t *)cpu_state->vmcb)[0] = vmcs_revision;

	vmx_make_active(cpu_state);

	vmwrite(VMXREG_16G_CS_SEL, 0x10);
	vmwrite(VMXREG_16G_DS_SEL, 0x18);
	vmwrite(VMXREG_16G_ES_SEL, 0x18);
	vmwrite(VMXREG_16G_FS_SEL, 0x18);
	vmwrite(VMXREG_16G_GS_SEL, 0x18);
	vmwrite(VMXREG_16G_SS_SEL, 0x18);
	vmwrite(VMXREG_16G_LDTR_SEL, 0x00);
	vmwrite(VMXREG_16G_TR_SEL, 0x00);
	vmwrite(VMXREG_16G_INTSTATUS, 0x00);

	vmwrite(VMXREG_16H_CS_SEL, 0x08);
	vmwrite(VMXREG_16H_DS_SEL, 0x10);
	vmwrite(VMXREG_16H_ES_SEL, 0x10);
	vmwrite(VMXREG_16H_FS_SEL, 0x10);
	vmwrite(VMXREG_16H_GS_SEL, 0x10);
	vmwrite(VMXREG_16H_SS_SEL, 0x10);
	vmwrite(VMXREG_16H_TR_SEL, 0x18);

	vmwrite(VMXREG_64C_APIC_ADDR, 0xfee00000);
	vmwrite(VMXREG_64C_EPTP, new_vm_cpu->pagetable_base | (0x3 << 3));

	vmwrite(VMXREG_64G_VMCS_LINK, 0xffffffffffffffffL);
	vmwrite(VMXREG_64G_IA32_EFER, 0x500);
	vmwrite(VMXREG_64G_IA32_PAT, 0x0007040600070406L);

	vmwrite(VMXREG_64H_IA32_EFER, 0xd00);
	vmwrite(VMXREG_64H_IA32_PAT, 0x0007040600070406L);

	vmwrite(VMXREG_32G_CS_LIMIT, 0xffffffff);
	vmwrite(VMXREG_32G_DS_LIMIT, 0xffffffff);
	vmwrite(VMXREG_32G_ES_LIMIT, 0xffffffff);
	vmwrite(VMXREG_32G_FS_LIMIT, 0xffffffff);
	vmwrite(VMXREG_32G_GS_LIMIT, 0xffffffff);
	vmwrite(VMXREG_32G_SS_LIMIT, 0xffffffff);
	vmwrite(VMXREG_32G_LDTR_LIMIT, 0x0);
	vmwrite(VMXREG_32G_TR_LIMIT, 0x0);
	vmwrite(VMXREG_32G_GDTR_LIMIT, 0x1f);
	vmwrite(VMXREG_32G_IDTR_LIMIT, 0x0);
	vmwrite(VMXREG_32G_CS_ACCESS, 0xa09b);
	vmwrite(VMXREG_32G_DS_ACCESS, 0xa093);
	vmwrite(VMXREG_32G_ES_ACCESS, 0xa093);
	vmwrite(VMXREG_32G_FS_ACCESS, 0xa093);
	vmwrite(VMXREG_32G_GS_ACCESS, 0xa093);
	vmwrite(VMXREG_32G_SS_ACCESS, 0xa093);
	vmwrite(VMXREG_32G_LDTR_ACCESS, 0x10000);  // unusable
	vmwrite(VMXREG_32G_TR_ACCESS, 0x8b);
	vmwrite(VMXREG_32G_INTSTATE, 0x0);
	vmwrite(VMXREG_32G_ACTSTATE, 0x0);

	vmwrite(VMXREG_NWC_CR0_MASK, 0xffffffff);
	vmwrite(VMXREG_NWC_CR4_MASK, 0xffffffff);
	cpu_state->guest_cr0 = 0x80000021;
	cpu_state->guest_cr3 = 0xf6000;
	cpu_state->guest_cr4 = 0x2030;
	vmwrite(VMXREG_NWC_CR0_READ_SHADOW, cpu_state->guest_cr0);
	vmwrite(VMXREG_NWC_CR4_READ_SHADOW, cpu_state->guest_cr4);

	vmwrite(VMXREG_NWG_CR0, 0x80000021);
	vmwrite(VMXREG_NWG_CR3, 0xf6000);
	vmwrite(VMXREG_NWG_CR4, 0x2030);
	vmwrite(VMXREG_NWG_CS_BASE, 0x0);
	vmwrite(VMXREG_NWG_DS_BASE, 0x0);
	vmwrite(VMXREG_NWG_ES_BASE, 0x0);
	vmwrite(VMXREG_NWG_FS_BASE, 0x0);
	vmwrite(VMXREG_NWG_GS_BASE, 0x0);
	vmwrite(VMXREG_NWG_SS_BASE, 0x0);
	vmwrite(VMXREG_NWG_LDTR_BASE, 0x0);
	vmwrite(VMXREG_NWG_TR_BASE, 0x0);
	vmwrite(VMXREG_NWG_GDTR_BASE, 0x8ff00);
	vmwrite(VMXREG_NWG_IDTR_BASE, 0x0);

	vmwrite(VMXREG_NWG_RIP, 0x100000);
	vmwrite(VMXREG_NWG_RSP, 0xfc000);
	vmwrite(VMXREG_NWG_RFLAGS, 0x2);

	vmwrite(VMXREG_NWH_CR0, 0x80010031);
	vmwrite(VMXREG_NWH_CR3, _specification.cpus[cpu_number].pagetable_address);
	vmwrite(VMXREG_NWH_CR4, 0x20b0);
	vmwrite(VMXREG_NWH_FS_BASE, 0x0);
	vmwrite(VMXREG_NWH_GS_BASE, 0x0);
	vmwrite(VMXREG_NWH_TR_BASE, (uintptr_t)_specification.arch_specification->tss);
	vmwrite(VMXREG_NWH_GDTR_BASE, (uintptr_t)_specification.arch_specification->gdt);
	vmwrite(VMXREG_NWH_IDTR_BASE, (uintptr_t)&__boot + 0x200);

	vmwrite(VMXREG_NWH_RIP, (uintptr_t)&vmx_exit_asm);
	vmwrite(VMXREG_NWH_RSP, top_of_stack);

	cpu_state->rsi = 0xfc000;

	vmwrite(VMXREG_32C_PINBASED, 0x16 | 0x29); // fixed + {0,3,5}
	vmwrite(VMXREG_32C_PRIMARY, 0x401e172 | 0x81000080); // fixed + {7,24,31}
	vmwrite(VMXREG_32C_SECONDARY, 0x1002); // {1,12}
	vmwrite(VMXREG_32C_ENTRYCONTROL, 0x11ff | 0xc200); // fixed + {9,14,15}
	vmwrite(VMXREG_32C_EXITCONTROL, 0x36dff | 0x3c8200); // fixed + {9,[15!],18,19,20,21}

	vmwrite(VMXREG_32C_EXCMAP, 0x0);	// do not catch any guest exceptions
}

static const uint32_t reg_idx[] = { 0, 2, 3, 1, ~0, 6, 4, 5,
				7, 8, 9, 10, 11, 12, 13, 14 };
void handle_cr_read(uint64_t qual) {
	uint64_t gpr_value;

	if ((qual & 0xf) == 0x0) {
		gpr_value = vmread(VMXREG_NWG_CR0);
		// vmwrite(VMXREG_NWC_CR0_READ_SHADOW, gpr_value);
	} else if ((qual & 0xf) == 0x3) {
		gpr_value = vmread(VMXREG_NWG_CR3);
	} else if ((qual & 0xf) == 0x4) {
		gpr_value = vmread(VMXREG_NWG_CR4);
	}

	// printf("RD CR%d (@%x): %x\r\n", qual & 0xf, vmread(VMXREG_NWG_RIP), gpr_value);

	if ((qual & 0xf00) == 0x400) {
		vmwrite(VMXREG_NWG_RSP, gpr_value);
	} else {
		(&current_vm_cpu->cpu_state->rax)[reg_idx[(qual & 0xf00) >> 8]] = gpr_value;
	}
}

void handle_cr_write(uint64_t qual) {
	uint64_t gpr_value;

	if ((qual & 0xf00) == 0x400) {
		gpr_value = vmread(VMXREG_NWG_RSP);
	} else {
		gpr_value = (&current_vm_cpu->cpu_state->rax)[reg_idx[(qual & 0xf00) >> 8]];
	}

	// printf("WR CR%d (@%x): %x\r\n", qual & 0xf, vmread(VMXREG_NWG_RIP), gpr_value);

	if ((qual & 0xf) == 0x0) {
		current_vm_cpu->cpu_state->guest_cr0 = gpr_value;
		vmwrite(VMXREG_NWC_CR0_READ_SHADOW, current_vm_cpu->cpu_state->guest_cr0);
		vmwrite(VMXREG_NWG_CR0, gpr_value | 0x80000021);
		if (current_vm_cpu->cpu_state->guest_cr0 & 0x80000000) {
			vmwrite(VMXREG_NWG_CR3, current_vm_cpu->cpu_state->guest_cr3);
			vmwrite(VMXREG_NWG_CR4, current_vm_cpu->cpu_state->guest_cr4 | 0x2010);
		}
	} else if ((qual & 0xf) == 0x3) {
		current_vm_cpu->cpu_state->guest_cr3 = gpr_value;
		if (current_vm_cpu->cpu_state->guest_cr0 & 0x80000000)
			vmwrite(VMXREG_NWG_CR3, gpr_value);
	} else if ((qual & 0xf) == 0x4) {
		current_vm_cpu->cpu_state->guest_cr4 = gpr_value;
		vmwrite(VMXREG_NWC_CR4_READ_SHADOW, current_vm_cpu->cpu_state->guest_cr4);
		if (!(current_vm_cpu->cpu_state->guest_cr0 & 0x80000000))
			gpr_value &= ~0x20;
		vmwrite(VMXREG_NWG_CR4, gpr_value | 0x2010);
		gpr_value = current_vm_cpu->cpu_state->guest_cr4;
	}
}

static uint32_t eptv_flags(uint64_t qual) {
	uint32_t flags = 0;

	if (qual & 1)	flags |= MEMAREA_FLAG_R;
	if (qual & 2)	flags |= MEMAREA_FLAG_W;
	if (qual & 4)	flags |= MEMAREA_FLAG_X;

	return flags;
}

void vm_cpu_vmexit(void) {
	vm_cpu_state *xstate = current_vm_cpu->cpu_state;
	uint32_t ex_reason = vmread(VMXREG_32E_EXIT_REASON);
	uint64_t ex_qual = vmread(VMXREG_NWE_EXITQUAL);

	// if (ex_reason != 0x1e) printf("--> #VMEXIT %x (q %x)\r\n", ex_reason, ex_qual);

	switch (ex_reason) {
	case 0x00:		// caught exception
		printf("Guest Exception: %x %x %x %x (%x)\r\n",
			vmread(VMXREG_32E_EXIT_INTINFO), vmread(VMXREG_32E_EXIT_INT_EC),
			vmread(VMXREG_32E_IDT_VECINFO), vmread(VMXREG_32E_IDT_VEC_EC),
			ex_qual);
		vm_cpu_dump_state();
		vm_cpu_dump_stack(current_vm_cpu);
		panic();
	case 0x01:		// Ext. IRQ
		if (0) {
			uint32_t i;
			volatile uint32_t *xv = (void *)core_memarea(MEMAREA_LAPIC)->vaddr;
			printf("External IRQ (INTINFO: %x)\r\n", vmread(VMXREG_32E_EXIT_INTINFO));
			printf("ISRs: %x %x %x %x\r\n", xv[0x100>>2], xv[0x110>>2], xv[0x120>>2], xv[0x130>>2]);
			printf("ISRs: %x %x %x %x\r\n", xv[0x140>>2], xv[0x150>>2], xv[0x160>>2], xv[0x170>>2]);
			printf("IRRs: %x %x %x %x\r\n", xv[0x200>>2], xv[0x210>>2], xv[0x220>>2], xv[0x230>>2]);
			printf("IRRs: %x %x %x %x\r\n", xv[0x240>>2], xv[0x250>>2], xv[0x260>>2], xv[0x270>>2]);
			printf("TPR/PPR (Spur): %x %x %x\r\n", xv[0x80>>2], xv[0xa0>>2], xv[0xf0>>2]);
			printf("LVTs: %x %x %x %x\r\n", xv[0x320>>2], xv[0x330>>2], xv[0x340>>2], xv[0x350>>2]);
			printf("LVTs: %x %x %x %x\r\n", xv[0x360>>2], xv[0x370>>2], xv[0x2f0>>2], xv[0x280>>2]);
			xv = (void *)core_memarea(MEMAREA_IOAPIC)->vaddr;
			for (i = 0; i < 23; i++) {
				xv[0] = 0x10 + 2*i;
				printf("IORED: %x\r\n", xv[0x10>>2]);
			}
			asm volatile("mov $0x21, %%rdx\n\tinb (%%dx), %%al\n\tmov %%rax, %%rcx\n\tmov $0xa1, %%rdx\n\tinb (%%dx), %%al\n\tshl $8, %%rax\n\tor %%rax, %%rcx" : "=c" (i) :: "rax", "rdx");
			printf("PIC ISR: %x\r\n", i);
			asm volatile("mov $0x20, %%eax\n\tmov $0x20, %%edx\n\toutb %%al, (%%dx)\n\tmov $0xa0, %%edx\n\toutb %%al, (%%dx)" ::: "eax", "edx");
		}
		_incoming_idt_vector = vmread(VMXREG_32E_EXIT_INTINFO) & 0xff;
		// vm_cpu_dump_state();
		interrupt_handler();
		break;
	case 0x07:		// interrupt window
		vmwrite(VMXREG_32C_PRIMARY, 0x401e172 | 0x81000080); // disable int window
		/* upcall will attempt injection automatically */
		upcall();
	case 0x0a:		// CPUID
		{ uint32_t fixup = 0;
		  // printf("CPUID %x(%x)\r\n", xstate->rax, xstate->rcx);
		  if (xstate->rax == 0x1) fixup = 1;
		  if (xstate->rax == 0x6) fixup = 6;
		  cpuid(&xstate->rax, xstate->rax, xstate->rcx);
		  // printf("CPUID -> %x %x %x %x\r\n", xstate->rax, xstate->rbx, xstate->rcx, xstate->rdx);
		  switch (fixup) {
		  case 1: xstate->rcx &= ~0xfffa0008; break;
		  case 6: xstate->rax &= ~0x00000f80; break;
		  default: break;
		  }
		}
		break;
	case 0x0c:		// HLT
		// vm_cpu_dump_state();
		// vm_cpu_dump_stack(current_vm_cpu);
		vmwrite(VMXREG_NWG_RIP, vmread(VMXREG_NWG_RIP) + vmread(VMXREG_32E_INSN_LENGTH));
		vmwrite(VMXREG_32G_INTSTATE, 0);
		sched_state_modify(current_vm_cpu->sched_entities, SCHEDULER_STATE_WAITING);
		/* never reached */
		break;
	case 0x1c:		// CR access
		if ((ex_qual & 0x30) == 0x00) handle_cr_write(ex_qual);
		else if ((ex_qual & 0x30) == 0x10) handle_cr_read(ex_qual);
		else panic();
		break;
	case 0x1e:		// I/O
		if (((ex_qual >> 16) & 0xfff8) == 0x3f8) {
			// UART
			const emulate *emul = find_emulate_by_va(current_vm_cpu->emulate_tree, 0xffff03f8);
			if (ex_qual & 8) {
				xstate->rax = emulate_peripheral_load(current_vm_cpu, emul, 0xffff03f8 + ((ex_qual >> 16) & 7));
			} else {
				emulate_peripheral_store(current_vm_cpu, emul, 0xffff03f8 + ((ex_qual >> 16) & 7), xstate->rax);
			}
		} else if (((ex_qual >> 16) & 0xff7e) == 0x20) {
			printf("PIC Access: (%x -> %x)\r\n", xstate->rax, ex_qual);
			if (ex_qual & 8) xstate->rax = 0xffff;
		} else if (((ex_qual >> 16) & 0xffff) == 0x71) {
			if (ex_qual & 8) xstate->rax = 0;
		} else
			if (ex_qual & 8) xstate->rax = 0xffff;
		break;
	case 0x1f:		// RDMSR
		rdmsr(&xstate->rdx, &xstate->rax, xstate->rcx);
		// printf("RDMSR: %x:%x <- %x\r\n", xstate->rdx, xstate->rax, xstate->rcx);
		break;
	case 0x20:		// WRMSR
		if (xstate->rcx == 0xc0000080) {	// EFER
			if (!(xstate->guest_cr0 & 0x80000000))
				xstate->rax &= ~0x500;
			vmwrite(VMXREG_64G_IA32_EFER, (xstate->rdx << 32) | xstate->rax);
		} else if (xstate->rcx == 0xc0000100) {
			vmwrite(VMXREG_NWG_FS_BASE, (xstate->rdx << 32) | xstate->rax);
		} else if (xstate->rcx == 0xc0000101) {
			vmwrite(VMXREG_NWG_GS_BASE, (xstate->rdx << 32) | xstate->rax);
		} else if ((xstate->rcx >= 0xc0000081) && (xstate->rcx <= 0xc0000084)) {	// IA32_*STAR
			asm volatile("wrmsr" :: "c" (xstate->rcx), "a" (xstate->rax), "d" (xstate->rdx));
		} else {
			// printf("WRMSR ignored: %x:%x -> %x\r\n", xstate->rdx, xstate->rax & 0xffffffff, xstate->rcx);
		}
		break;
	case 0x30:		// EPT violation
		// printf("#EPT: %x / %x\r\n", vmread(VMXREG_NWE_GUEST_LADDR), vmread(VMXREG_64E_GPHYS));
		// vm_cpu_dump_stack(current_vm_cpu);
		vm_cpu_fault_nested(vmread(VMXREG_64E_GPHYS), eptv_flags(ex_qual));
		/* doesn't get here, vm_cpu_emulate_faulted() upcalls */
		break;
#if 0
	case 0x37:		// XSETBV
		//asm volatile("xsetbv" :: "a" (xstate->rax), "d" (xstate->rdx), "c" (xstate->rcx));
		break;
#endif
	default:
		printf("Unknown exit reason: %x\r\n", ex_reason);
		vm_cpu_dump_state();
		panic();
	}

	vmwrite(VMXREG_NWG_RIP, vmread(VMXREG_NWG_RIP) + vmread(VMXREG_32E_INSN_LENGTH));
	upcall();
}

static int is_register_read_insn(const uint8_t *bytes) {
	const uint8_t *b = bytes;

	if (*b == 0x44) b++;

	if (*b == 0x8b) return 1;
	if (*b == 0x89) return 0;

	printf("Undecidable insn! %x\r\n", vmread(VMXREG_NWG_RIP));
	panic();
}

void vm_cpu_emulate_faulted(const struct vm_cpu_ *vm_cpu_emulate, const struct emulate_ *em, uintptr_t addr) {
	vm_cpu_state *xstate = vm_cpu_emulate->cpu_state;
	vm_cpu_state_vmcb *vmcb = xstate->vmcb;
	uint64_t tmp_value;
	uintptr_t walkedAddr, base, size;
	uint32_t flags;
	uint32_t len;

	base = size = 0; flags = 0;
	walkedAddr = walker_x86_long(vm_cpu_emulate, vmread(VMXREG_NWG_CR3), vmread(VMXREG_NWG_RIP),
				&base, &size, &flags);
	// printf("Walked: %x +%x fl %x = final %x\r\n", base, size, flags, walkedAddr);

	fetch_bytes(vm_cpu_emulate, vmcb->bytes_fetched, walkedAddr, 8);
	// printf("Fetched Insn: %x\r\n", *(uint64_t *)vmcb->bytes_fetched);

	if (is_register_read_insn(vmcb->bytes_fetched)) {
			// mov to gpreg -> it's a read
			tmp_value = emulate_peripheral_load(vm_cpu_emulate, em, addr);
			x86_modrm_write_reg(vm_cpu_emulate, vmcb->bytes_fetched, tmp_value);
	} else {
			// mov from gpreg -> it's a write
			tmp_value = x86_modrm_read_reg(vm_cpu_emulate, vmcb->bytes_fetched);
			emulate_peripheral_store(vm_cpu_emulate, em, addr, tmp_value);
	}

	len = insn_length(vmcb->bytes_fetched);

	vmwrite(VMXREG_NWG_RIP, vmread(VMXREG_NWG_RIP) + len);
	upcall();
}

/* VM Event Injection */

void vm_cpu_inject_undefined(const struct vm_cpu_ *vm_cpu_inject) {
	(void)vm_cpu_inject;
}
void vm_cpu_inject_supervisorcall(const struct vm_cpu_ *vm_cpu_inject) {
	(void)vm_cpu_inject;
}
void vm_cpu_inject_fault(const struct vm_cpu_ *vm_cpu_inject, uintptr_t address, uint32_t flags) {
	(void)vm_cpu_inject; (void)address; (void)flags;
}
void vm_cpu_inject_interrupt(const struct vm_cpu_ *vm_cpu_inject, uint32_t interrupt_number) {
	uint64_t injection = vmread(VMXREG_32C_ENTRY_INTINFO);
	uint64_t guest_rflags = vmread(VMXREG_NWG_RFLAGS);

	(void)vm_cpu_inject;

	if (0 && (interrupt_number != 0xef))
		printf("Inj %x? @%x (fl %x)\r\n", interrupt_number, vmread(VMXREG_NWG_RIP), guest_rflags);

	if ((injection & 0x80000000) || !(guest_rflags & 0x200)) {
		/* already injection in progress or guest is not ready */
		vmwrite(VMXREG_32C_PRIMARY, 0x401e172 | 0x81000084); // activate int window exit
		return;
	}

	vmwrite(VMXREG_32C_ENTRY_INTINFO, 0x80000000 | interrupt_number);
}
