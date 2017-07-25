#define		TLB_ALL_ADDRESSES	~0U

extern uintptr_t top_of_stack;

extern void cpu_idle(void);
extern void cpu_zombie(void);

extern void tlb_flush(uintptr_t address, uint32_t identifier);

static inline void cpu_dcache_flush(int do_clean, int do_inval) {
	(void)do_clean;
	(void)do_inval;
}

static inline void arch_barrier() {
	asm volatile("mfence" ::: "memory");
}

extern void cpuid(uint64_t *, uint64_t, uint64_t);
extern void rdmsr(uint64_t *, uint64_t *, uint64_t);
