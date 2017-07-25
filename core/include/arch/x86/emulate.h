#ifndef	_PHIDIAS_ARCH_X86_EMULATE_H_
#define	_PHIDIAS_ARCH_X86_EMULATE_H_

void fetch_bytes(const vm_cpu *xcpu, uint8_t *space, uint64_t ip, uint32_t len);
void x86_modrm_write_reg(const vm_cpu *xcpu, uint8_t *bytes, uint64_t value);
uint64_t x86_modrm_read_reg(const vm_cpu *xcpu, uint8_t *bytes);
uint32_t insn_length(const uint8_t *bytes);

#endif
