#include <phidias.h>
#include <schedule.h>
#include <vm.h>
#include <misc_ops.h>
#include <arch/cpu_state.h>
#include <emulate/core.h>
#include <emulate/vtlb.h>
#include <arch/emulate.h>

static uintptr_t walk_simple(const vm_cpu *xcpu, uintptr_t addr) {
	uintptr_t base, size;
	uint32_t flags;

	return emulate_vtlb_walk(xcpu, addr, &base, &size, &flags);
}

void vm_cpu_dump_state(const struct vm_cpu_ *new_vm_cpu) {
	vm_cpu_state *cpu_state = new_vm_cpu->cpu_state;

	printf("EAX/EBX: %8x %8x\r\n", (uint32_t)cpu_state->vmcb->rax, (uint32_t)cpu_state->rbx);
	printf("ECX/EDX: %8x %8x\r\n", (uint32_t)cpu_state->rcx, (uint32_t)cpu_state->rdx);
	printf("ESI/EDI: %8x %8x\r\n", (uint32_t)cpu_state->rsi, (uint32_t)cpu_state->rdi);
	printf("EBP/ESP: %8x %8x\r\n", (uint32_t)cpu_state->rbp, (uint32_t)cpu_state->vmcb->rsp);
}

void vm_cpu_initialize(const struct vm_cpu_ *new_vm_cpu) {
	vm_cpu_state *cpu_state = new_vm_cpu->cpu_state;
	const memarea *arch_memarea = find_memarea_by_va(_specification.cpus[cpu_number].memarea_tree, (uintptr_t)cpu_state);

	memset(cpu_state, 0, sizeof(vm_cpu_state));

	cpu_state->vmcb = ((void *)cpu_state) + 0x1000;
	cpu_state->vmcb_pa = arch_memarea->paddr + 0x1000;

	memset(cpu_state->vmcb, 0, sizeof(vm_cpu_state_vmcb));
	cpu_state->vmcb->msrpm_base_pa = arch_memarea->paddr + 0x2000;
	memset((void *)cpu_state + 0x2000, 0xff, 0x2000);
	cpu_state->vmcb->iopm_base_pa = arch_memarea->paddr + 0x4000;
	memset((void *)cpu_state + 0x4000, 0xff, 0x2004);

	cpu_state->vmcb->intercept_cr = 0x00190019;
	cpu_state->vmcb->intercept_exceptions = 0x400a;
	cpu_state->vmcb->intercept_insn0 = 0x87200011;
	cpu_state->vmcb->intercept_insn1 = 0x0000007f;
	cpu_state->vmcb->rflags = 0x002;
	cpu_state->vmcb->rip = new_vm_cpu->vm->boot_address;
	cpu_state->vmcb->cr0 = 0x80000001;
	cpu_state->vmcb->cr4 = 0x00000090;
	cpu_state->vmcb->cs_sel = 0x8;
	cpu_state->vmcb->cs_attrib = 0xc9b;
	cpu_state->vmcb->cs_limit = 0xffffffff;
	cpu_state->vmcb->ds_sel = 0x10;
	cpu_state->vmcb->ds_attrib = 0xc93;
	cpu_state->vmcb->ds_limit = 0xffffffff;
	cpu_state->vmcb->es_sel = 0x10;
	cpu_state->vmcb->es_limit = 0xffffffff;
	cpu_state->vmcb->es_attrib = 0xc93;
	cpu_state->vmcb->ss_sel = 0x10;
	cpu_state->vmcb->ss_limit = 0xffffffff;
	cpu_state->vmcb->ss_attrib = 0xc93;
	cpu_state->vmcb->gdtr_base = 0x8ff00;
	cpu_state->vmcb->gdtr_limit = 0x1f;
	cpu_state->vmcb->asid = 1;

	cpu_state->guest_cr0 = 0x1;

	cpu_state->rsi = 0xfc400;
}

static uint32_t pfec_to_flags(uint64_t pf_error_code) {
	uint32_t flags = MEMAREA_FLAG_R;

	if (pf_error_code & 2)		flags |= MEMAREA_FLAG_W;
	if (pf_error_code & 4)		flags |= MEMAREA_FLAG_U;
	if (pf_error_code & 0x10)	flags |= MEMAREA_FLAG_X;

	return flags;
}

void vm_cpu_vmexit(void) {
	vm_cpu_state *xstate = current_vm_cpu->cpu_state;
	vm_cpu_state_vmcb *vmcb = xstate->vmcb;
	uint32_t parsed_flags;
	uint64_t gphys_rip;
	uint64_t tmp_value;

	printf("--> #VMEXIT");
	printf(" R %x | %x %x", (uint32_t)vmcb->exitcode, (uint32_t)vmcb->exitinfo1, (uint32_t)vmcb->exitinfo2);
	printf(" @ [%x/%x]\r\n", vmcb->cs_sel, (uint32_t)vmcb->rip);
	if (vmcb->rip == 0x100076) { printf("QEMU BUG, time to roll over and die...\r\n"); panic(); }
	xstate->rax = vmcb->rax;

	switch (vmcb->exitcode) {
	case 0x00:
		gphys_rip = walk_simple(current_vm_cpu, vmcb->rip);
		fetch_bytes(current_vm_cpu, vmcb->bytes_fetched, gphys_rip, 3);
		x86_modrm_write_reg(current_vm_cpu, vmcb->bytes_fetched[2], xstate->guest_cr0);
		vmcb->rip += 3;
		break;
	case 0x03:
		gphys_rip = walk_simple(current_vm_cpu, vmcb->rip);
		fetch_bytes(current_vm_cpu, vmcb->bytes_fetched, gphys_rip, 3);
		x86_modrm_write_reg(current_vm_cpu, vmcb->bytes_fetched[2], xstate->guest_cr3);
		vmcb->rip += 3;
		break;
	case 0x04:
		gphys_rip = walk_simple(current_vm_cpu, vmcb->rip);
		fetch_bytes(current_vm_cpu, vmcb->bytes_fetched, gphys_rip, 3);
		x86_modrm_write_reg(current_vm_cpu, vmcb->bytes_fetched[2], xstate->guest_cr4);
		vmcb->rip += 3;
		break;
	case 0x10:
		gphys_rip = walk_simple(current_vm_cpu, vmcb->rip);
		fetch_bytes(current_vm_cpu, vmcb->bytes_fetched, gphys_rip, 3);
		if (vmcb->bytes_fetched[1] == 0x06) {
			xstate->guest_cr0 &= ~0x10;
			vmcb->rip += 2;
			break;
		}
		tmp_value = x86_modrm_read_reg(current_vm_cpu, vmcb->bytes_fetched[2]);
#ifdef EMULATE_VTLB
		if ((tmp_value & 0x80000000) && !(xstate->guest_cr0 & 0x80000000)) {
			emulate_vtlb_switch(current_vm_cpu, xstate->guest_cr3, (uint32_t)(xstate->guest_cr3 >> 12));
		} else if (!(tmp_value & 0x80000000) && (xstate->guest_cr0 & 0x80000000)) {
			emulate_vtlb_switch(current_vm_cpu, VTLB_INVALID_PAGETABLE, ~0U);
		}
#endif
		xstate->guest_cr0 = tmp_value;
		vmcb->rip += 3;
		break;
	case 0x13:
		gphys_rip = walk_simple(current_vm_cpu, vmcb->rip);
		fetch_bytes(current_vm_cpu, vmcb->bytes_fetched, gphys_rip, 3);
		xstate->guest_cr3 = x86_modrm_read_reg(current_vm_cpu, vmcb->bytes_fetched[2]);
#ifdef EMULATE_VTLB
		if (xstate->guest_cr0 & 0x80000000) {
			emulate_vtlb_switch(current_vm_cpu, xstate->guest_cr3, (uint32_t)(xstate->guest_cr3 >> 12));
		}
#endif
		vmcb->rip += 3;
		break;
	case 0x14:
		gphys_rip = walk_simple(current_vm_cpu, vmcb->rip);
		fetch_bytes(current_vm_cpu, vmcb->bytes_fetched, gphys_rip, 3);
		tmp_value = x86_modrm_read_reg(current_vm_cpu, vmcb->bytes_fetched[2]);
#ifdef EMULATE_VTLB
		if ((tmp_value ^ xstate->guest_cr4) & 0x90) {
			emulate_vtlb_flush_current(current_vm_cpu);
		}
#endif
		xstate->guest_cr4 = tmp_value;
		vmcb->rip += 3;
		break;
	case 0x4e:
		parsed_flags = pfec_to_flags(vmcb->exitinfo1);
		vm_cpu_fault((uintptr_t)vmcb->exitinfo2, parsed_flags, (uint32_t)vmcb->exitinfo1);
		break;
	case 0x79:
		gphys_rip = walk_simple(current_vm_cpu, vmcb->rip);
		fetch_bytes(current_vm_cpu, vmcb->bytes_fetched, gphys_rip, 3);
		tmp_value = x86_modrm_read_reg(current_vm_cpu, vmcb->bytes_fetched[2]);
#ifdef EMULATE_VTLB
		emulate_vtlb_unmap_by_identifier(current_vm_cpu, (uint32_t)(xstate->guest_cr3 >> 12), tmp_value);
#endif
		vmcb->rip += 3;
		break;
	default:
		panic();
	}

	vmcb->rax = xstate->rax;
	upcall();
}

void vm_cpu_emulate_faulted(const struct vm_cpu_ *vm_cpu_emulate, const struct emulate_ *em, uintptr_t addr) {
	vm_cpu_state *xstate = vm_cpu_emulate->cpu_state;
	vm_cpu_state_vmcb *vmcb = xstate->vmcb;
	uint64_t tmp_value;

	fetch_bytes(vm_cpu_emulate, vmcb->bytes_fetched, walk_simple(vm_cpu_emulate, vmcb->rip), 8);
	if (vmcb->bytes_fetched[0] == 0x8b) {
			// mov to gpreg -> it's a read
			tmp_value = emulate_peripheral_load(vm_cpu_emulate, em, addr);
			x86_modrm_write_reg(vm_cpu_emulate, vmcb->bytes_fetched[1], tmp_value);
			vmcb->rip += 2;
	} else if (vmcb->bytes_fetched[0] == 0x89) {
			// mov from gpreg -> it's a write
			tmp_value = x86_modrm_read_reg(vm_cpu_emulate, vmcb->bytes_fetched[1]);
			emulate_peripheral_store(vm_cpu_emulate, em, addr, tmp_value);
			vmcb->rip += 2;
	} else {
		printf("EmF %x\r\n", addr);
		panic();
	}
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
	(void)vm_cpu_inject; (void)interrupt_number;
}
