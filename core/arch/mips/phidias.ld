PHDRS {
	load_x PT_LOAD;
	load_r PT_LOAD;
	load_ws PT_LOAD;
	load_w PT_LOAD;
}
SECTIONS {
	. = PHIDIAS_VADDR_BASE;
	__phidias_start = .;
	.text : {
			__phidias_code_start = .;
			*(.text.startup)
			*(.text)
			*(.text.__x86*)
			*(.text.unlikely)
			__phidias_code_end = .;
			. = ALIGN(0x1000);
	} :load_x
	.tlb_handlers : {
			__phidias_tlb_handlers_start = .;
			*(.tlb_handler)
			__phidias_tlb_handlers_end = .;
	} :load_x
	.exc_handlers : {
		. = . + (0x180 - SIZEOF( .tlb_handlers ));
		__phidias_exc_handlers_start = .;
		*(.exc_handler)
		__phidias_exc_handlers_end = .;
	} :load_x
	.spc_handlers : {
		. = . + (0x200 - SIZEOF( .exc_handlers ) - SIZEOF( .tlb_handlers ));
		__phidias_spc_handlers_start = .;
		*(.spc_handler)
		*(.isr)
		__phidias_spc_handlers_end = .;
		. = ALIGN(0x1000);
	} :load_x
	.rodata : {
			__phidias_rodata_start = .;
			*(.rodata)
			*(.rodata.str1.1)
			*(.rodata.str1.4)
			*(.rodata.str1.8)
			*(.rodata.cst8)
			__phidias_rodata_end = .;
			. = ALIGN(0x1000);
        } :load_r
	.data_shared : {
			__phidias_data_shared_start = .;
			_gp = .;
			*(.data_shared)
			__phidias_data_shared_end = .;
			. = ALIGN(0x1000);
        } :load_ws
	.data : {
			__phidias_data_start = .;
			*(.data)
			__phidias_data_end = .;
	} :load_w
	.bss : {
			__phidias_bss_start = .;
			*(.bss)
			__phidias_bss_end = .;
			. = ALIGN(0x1000);
        } :load_w
	__phidias_end = .;
	_specification = .;
	/DISCARD/ : {
			*(.comment)
			*(.eh_frame)
			*(.eh_frame_hdr)
			*(.MIPS.abiflags)
	} :NONE
}
ENTRY(__boot)
