#include <phidias.h>
#include <vm.h>
#include <misc_ops.h>
#include <arch/cpu_state.h>

void fetch_bytes(const vm_cpu *xcpu, uint8_t *space, uint64_t ip, uint32_t len) {
	const memarea *ip_area;

	ip_area = find_memarea_by_va(xcpu->memarea_tree, ip);

	if ((ip_area == NULL) || (ip_area->reference_area == NULL)) {
		printf("fetch_bytes() failed\r\n");
		panic();
	}

	ip = (ip - ip_area->vaddr);

	memcpy(space, (void *)(ip_area->reference_area->vaddr + (uint32_t)ip), len);
}

void x86_modrm_write_reg(const vm_cpu *xcpu, uint8_t *bytes, uint64_t value) {
	vm_cpu_state *xstate = xcpu->cpu_state;
	uint32_t gpr_index, modrm_byte;

	if (bytes[0] == 0x44) {
		gpr_index = 8;
		modrm_byte = bytes[2];
	} else {
		gpr_index = 0;
		modrm_byte = bytes[1];
	}
	gpr_index |= (modrm_byte >> 3) & 7;

	switch (gpr_index) {
	case 0:		xstate->rax = value; break;
	case 1:		xstate->rcx = value; break;
	case 2:		xstate->rdx = value; break;
	case 3:		xstate->rbx = value; break;
	//case 4:		vmcb->rax = value; break;
	//case 5:		vmcb->rax = value; break;
	case 6:		xstate->rsi = value; break;
	case 7:		xstate->rdi = value; break;
	case 8:		xstate->r8 = value; break;
	case 9:		xstate->r9 = value; break;
	case 10:	xstate->r10 = value; break;
	case 11:	xstate->r11 = value; break;
	case 12:	xstate->r12 = value; break;
	case 13:	xstate->r13 = value; break;
	case 14:	xstate->r14 = value; break;
	default: printf("modrm panic W %d", gpr_index); panic();
	}
}

uint64_t x86_modrm_read_reg(const vm_cpu *xcpu, uint8_t *bytes) {
	vm_cpu_state *xstate = xcpu->cpu_state;
	uint32_t gpr_index, modrm_byte;

	if (bytes[0] == 0x44) {
		gpr_index = 8;
		modrm_byte = bytes[2];
	} else {
		gpr_index = 0;
		modrm_byte = bytes[1];
	}
	gpr_index |= (modrm_byte >> 3) & 7;

	switch (gpr_index) {
	case 0:		return xstate->rax;
	case 1:		return xstate->rcx;
	case 2:		return xstate->rdx;
	case 3:		return xstate->rbx;
	//case 4:		vmcb->rax = value; break;
	//case 5:		vmcb->rax = value; break;
	case 6:		return xstate->rsi;
	case 7:		return xstate->rdi;
	case 8:		return xstate->r8;
	case 9:		return xstate->r9;
	case 10:	return xstate->r10;
	case 11:	return xstate->r11;
	case 12:	return xstate->r12;
	case 13:	return xstate->r13;
	case 14:	return xstate->r14;
	default: printf("modrm panic R %d", gpr_index); panic();
	}
}

uint32_t insn_length(const uint8_t *bytes) {
	const uint8_t *p = bytes;
	uint32_t len;

	if (p[0] == 0x44) {
		len = 1;
		p++;
	} else {
		len = 0;
	}

	/* only good as long as we don't have a REX prefix; see IA32 Vol2 p36 */
	switch (p[1] & 0xc0) {
	case 0x00:
		if ((p[1] & 0x7) == 0x4) {
			len += 3;
			if ((p[2] & 0x7) == 0x5)
				len += 4;
		} else if ((p[1] & 0x7) == 0x5)
			len += 6;
		else
			len += 2;
		break;
	case 0x40:
		if ((p[1] & 0x7) == 0x4) len += 4;
		else len += 3;
		break;
	case 0x80:
		if ((p[1] & 0x7) == 0x4) len += 7;
		else len += 6;
		break;
	case 0xc0:
		len += 2;
		break;
	}

	return len;
}
