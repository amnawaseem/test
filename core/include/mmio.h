#ifndef	_PHIDIAS_MMIO_H_
#define	_PHIDIAS_MMIO_H_

static inline void mmio_write32(uintptr_t address, uint32_t value) {
	volatile uint32_t *location = (volatile uint32_t *)address;

	*location = value;
}

static inline uint32_t mmio_read32(uintptr_t address) {
	volatile uint32_t *location = (volatile uint32_t *)address;

	return *location;
}

//

static inline void mmio_rmw32(uintptr_t address, uint32_t add, uint32_t del) {
	volatile uint32_t *location = (volatile uint32_t *)address;
	uint32_t value;

	value = *location;
	value = (value | add) & ~del;
	*location = value;
}

static inline void mmio_set32(uintptr_t address, uint32_t add) {
	mmio_rmw32(address, add, 0);
}

static inline void mmio_clear32(uintptr_t address, uint32_t del) {
	mmio_rmw32(address, 0, del);
}

#endif
