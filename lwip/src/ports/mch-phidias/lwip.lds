PHDRS {
	load PT_LOAD FILEHDR PHDRS;
}
SECTIONS {
	. = 0x40010000;
	__lwip_phidias_start = .;
	.assorted : {
		__lwip_phidias_code_start = .;
		*(.text.startup)
		*(.text)
		__lwip_phidias_code_end = .;
		. = ALIGN(0x40);
		__lwip_phidias_data_start = .;
		*(.rodata)
		*(.rodata.str1.1)
		*(.rodata.str1.4)
		*(.rodata.str1.8)
		*(.data)
		__lwip_phidias_data_end = .;
		. = ALIGN(0x40);
	} :load
	__lwip_phidias_bss_start = .;
	.bss : {
		*(.bss)
	} :load
	__lwip_phidias_bss_end = .;
	/DISCARD/ : {
		*(.comment)
		*(.eh_frame)
		*(.eh_frame_hdr)
	} :NONE
}
ENTRY(_start)
