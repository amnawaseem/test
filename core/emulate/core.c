#include <phidias.h>
#include <vm.h>
#include <emulate/core.h>
#include <emulate/uart.h>
#include <emulate/irq.h>
#include <emulate/timer.h>
#include <emulate/vtlb.h>
#include <emulate/memory.h>

const emulate *find_emulate_by_va(const tree_emulate *tree, uintptr_t vaddr) {
	while (tree) {
		if (tree->item->vaddr <= vaddr) {
			if (tree->item->vaddr + tree->item->size > vaddr) {
				return tree->item;
			} else {
				tree = tree->right;
			}
		} else {
			tree = tree->left;
		}
	}

	return NULL;
}

void emulate_initialize(const vm_cpu *vm_cpu_reset, const emulate *emu) {
	switch (emu->type) {
#ifdef EMULATE_UART_PL011
	case EMULATE_TYPE_UART_PL011:
		emulate_uart_pl011_initialize(vm_cpu_reset, emu->control.uart_pl011);
		break;
#endif
#ifdef EMULATE_UART_16550
	case EMULATE_TYPE_UART_16550:
		emulate_uart_16550_initialize(vm_cpu_reset, emu->control.uart_16550);
		break;
#endif
#ifdef EMULATE_UART_PIC32
	case EMULATE_TYPE_UART_PIC32:
		emulate_uart_pic32_initialize(vm_cpu_reset, emu->control.uart_pic32);
		break;
#endif
#ifdef EMULATE_VTLB
	case EMULATE_TYPE_VTLB:
		emulate_vtlb_initialize(vm_cpu_reset, emu->control.vtlb);
		break;
#endif
#ifdef EMULATE_TIMER_MPCORE
	case EMULATE_TYPE_TIMER_MPCORE:
		emulate_timer_mpcore_initialize(vm_cpu_reset, emu->control.timer_mpcore);
		break;
#endif
#ifdef EMULATE_TIMER_SP804
	case EMULATE_TYPE_TIMER_SP804:
		emulate_timer_sp804_initialize(vm_cpu_reset, emu->control.timer_sp804);
		break;
#endif
#ifdef EMULATE_TIMER_ARMCP14
	case EMULATE_TYPE_TIMER_ARMCP14:
		emulate_timer_armcp14_initialize(vm_cpu_reset, emu->control.timer_armcp14);
		break;
#endif
#ifdef EMULATE_IRQ_GIC
	case EMULATE_TYPE_IRQ_GIC:
	case EMULATE_TYPE_IRQ_GIC_VIRTEXT:
		emulate_irq_gic_initialize(vm_cpu_reset, emu->control.irq_gic);
		break;
#endif
#ifdef EMULATE_IRQ_LAPIC
	case EMULATE_TYPE_IRQ_LAPIC:
		emulate_irq_lapic_initialize(vm_cpu_reset, emu->control.irq_lapic);
		break;
#endif
#ifdef EMULATE_MEMORY32
	case EMULATE_TYPE_MEMORY_32BIT:
		emulate_memory_initialize(vm_cpu_reset, emu->control.memory);
		break;
#endif
	default:
		(void)vm_cpu_reset;
		printf("Cannot initialize unknown vdev %x!\r\n", emu->type);
	}
}

void emulate_initialize_all(const vm_cpu *vm_cpu_reset) {
	uint32_t i;

	for (i = 0; i < vm_cpu_reset->num_emulates; i++) {
		emulate_initialize(vm_cpu_reset, vm_cpu_reset->emulates+i);
	}
}

// --------------------------------------------------------------------------

uint32_t emulate_peripheral_load(const vm_cpu *vm_cpu_em, const emulate *emul, uintptr_t address) {
	uintptr_t offset = (address - emul->vaddr);
	uint32_t value;

	switch (emul->type) {
#ifdef EMULATE_UART_PL011
	case EMULATE_TYPE_UART_PL011:
		return emulate_uart_pl011_load(vm_cpu_em, emul->control.uart_pl011, emul->bar, offset);
#endif
#ifdef EMULATE_UART_16550
	case EMULATE_TYPE_UART_16550:
		return emulate_uart_16550_load(vm_cpu_em, emul->control.uart_16550, emul->bar, offset);
#endif
#ifdef EMULATE_UART_PIC32
	case EMULATE_TYPE_UART_PIC32:
		return emulate_uart_pic32_load(vm_cpu_em, emul->control.uart_pic32, emul->bar, offset);
#endif
#ifdef EMULATE_TIMER_MPCORE
	case EMULATE_TYPE_TIMER_MPCORE:
		return emulate_timer_mpcore_load(vm_cpu_em, emul->control.timer_mpcore, emul->bar, offset);
#endif
#ifdef EMULATE_TIMER_SP804
	case EMULATE_TYPE_TIMER_SP804:
		return emulate_timer_sp804_load(vm_cpu_em, emul->control.timer_sp804, emul->bar, offset);
#endif
#ifdef EMULATE_IRQ_GIC
	case EMULATE_TYPE_IRQ_GIC:
	case EMULATE_TYPE_IRQ_GIC_VIRTEXT:
		return emulate_irq_gic_load(vm_cpu_em, emul->control.irq_gic, emul->bar, offset);
#endif
#ifdef EMULATE_IRQ_LAPIC
	case EMULATE_TYPE_IRQ_LAPIC:
		return emulate_irq_lapic_load(vm_cpu_em, emul->control.irq_lapic, emul->bar, offset);
#endif
#ifdef EMULATE_MEMORY32
	case EMULATE_TYPE_MEMORY_32BIT:
		value = emulate_memory_load(vm_cpu_em, emul->control.memory, emul->bar, offset);
		// printf("MEM R from %08x, val %08x\r\n", address, value);
		return value;
#endif
	default:
		(void)vm_cpu_em; (void)value;
		printf("UNEMUL R %x @%x (%x)\r\n", emul->type, address, offset);
		return 0;
	}
}

void emulate_peripheral_store(const vm_cpu *vm_cpu_em, const emulate *emul, uintptr_t address, uint32_t value) {
	uintptr_t offset = (address - emul->vaddr);

	switch (emul->type) {
#ifdef EMULATE_UART_PL011
	case EMULATE_TYPE_UART_PL011:
		emulate_uart_pl011_store(vm_cpu_em, emul->control.uart_pl011, emul->bar, offset, value);
		return;
#endif
#ifdef EMULATE_UART_16550
	case EMULATE_TYPE_UART_16550:
		emulate_uart_16550_store(vm_cpu_em, emul->control.uart_16550, emul->bar, offset, value);
		return;
#endif
#ifdef EMULATE_UART_PIC32
	case EMULATE_TYPE_UART_PIC32:
		emulate_uart_pic32_store(vm_cpu_em, emul->control.uart_pic32, emul->bar, offset, value);
		return;
#endif
#ifdef EMULATE_TIMER_MPCORE
	case EMULATE_TYPE_TIMER_MPCORE:
		emulate_timer_mpcore_store(vm_cpu_em, emul->control.timer_mpcore, emul->bar, offset, value);
		return;
#endif
#ifdef EMULATE_TIMER_SP804
	case EMULATE_TYPE_TIMER_SP804:
		emulate_timer_sp804_store(vm_cpu_em, emul->control.timer_sp804, emul->bar, offset, value);
		return;
#endif
#ifdef EMULATE_IRQ_GIC
	case EMULATE_TYPE_IRQ_GIC:
	case EMULATE_TYPE_IRQ_GIC_VIRTEXT:
		emulate_irq_gic_store(vm_cpu_em, emul->control.irq_gic, emul->bar, offset, value);
		return;
#endif
#ifdef EMULATE_IRQ_LAPIC
	case EMULATE_TYPE_IRQ_LAPIC:
		emulate_irq_lapic_store(vm_cpu_em, emul->control.irq_lapic, emul->bar, offset, value);
		return;
#endif
#ifdef EMULATE_MEMORY32
	case EMULATE_TYPE_MEMORY_32BIT:
		// printf("MEM W  to  %08x, val %08x\r\n", address, value);
		emulate_memory_store(vm_cpu_em, emul->control.memory, emul->bar, offset, value);
		return;
#endif
	default:
		(void)*vm_cpu_em;
		(void)value;
		printf("UNEMUL W %x @%x (%x)\r\n", emul->type, address, offset);
	}
}

// --------------------------------------------------------------------------

static inline void emulate_peripheral_save_hardware_state(const vm_cpu *vm_cpu_em, const emulate *emul) {
	switch (emul->type) {
#ifdef EMULATE_IRQ_GIC
	case EMULATE_TYPE_IRQ_GIC_VIRTEXT:
		emulate_irq_gicve_save_state(vm_cpu_em, emul->control.irq_gic);
		return;
#endif
#ifdef EMULATE_TIMER_ARMCP14
	case EMULATE_TYPE_TIMER_ARMCP14:
		emulate_timer_armcp14_save_state(vm_cpu_em, emul->control.timer_armcp14);
		return;
#endif
	default:
		(void)*vm_cpu_em;
		printf("UNEMUL SHW %x\r\n", emul->type);
	}
}

static inline void emulate_peripheral_load_hardware_state(const vm_cpu *vm_cpu_em, const emulate *emul) {
	switch (emul->type) {
#ifdef EMULATE_IRQ_GIC
	case EMULATE_TYPE_IRQ_GIC_VIRTEXT:
		emulate_irq_gicve_load_state(vm_cpu_em, emul->control.irq_gic);
		return;
#endif
#ifdef EMULATE_TIMER_ARMCP14
	case EMULATE_TYPE_TIMER_ARMCP14:
		emulate_timer_armcp14_load_state(vm_cpu_em, emul->control.timer_armcp14);
		return;
#endif
	default:
		(void)*vm_cpu_em;
		printf("UNEMUL LHW %x\r\n", emul->type);
	}
}

void vm_cpu_save_emulates(const vm_cpu *vm_cpu_em) {
	uint32_t i;

	for (i = 0; i < vm_cpu_em->num_hw_emulates; i++) {
		emulate_peripheral_save_hardware_state(vm_cpu_em, vm_cpu_em->hw_emulates[i]);
	}
}

void vm_cpu_load_emulates(const vm_cpu *vm_cpu_em) {
	uint32_t i;

	for (i = 0; i < vm_cpu_em->num_hw_emulates; i++) {
		emulate_peripheral_load_hardware_state(vm_cpu_em, vm_cpu_em->hw_emulates[i]);
	}
}
