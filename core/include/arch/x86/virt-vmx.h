struct vm_cpu_state_;

void vmwrite(uint32_t encoding, uint64_t value);
uint64_t vmread(uint32_t encoding);

extern uint32_t vmcs_revision;

extern void vmx_make_active(struct vm_cpu_state_ *);
extern void vmx_exit_asm(void);
