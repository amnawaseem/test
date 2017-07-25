#ifndef	__ASSEMBLER__

extern char __phidias_start[];

extern char __phidias_code_start[], __phidias_code_end[];
extern char __phidias_rodata_start[], __phidias_rodata_end[];
extern char __phidias_data_start[], __phidias_data_end[];
extern char __phidias_bss_start[], __phidias_bss_end[];
extern char __phidias_data_shared_start[], __phidias_data_shared_end[];

extern char __phidias_end[];

#else

.extern	__phidias_start

.extern __phidias_code_start
.extern __phidias_code_end
.extern __phidias_rodata_start
.extern __phidias_rodata_end
.extern __phidias_data_start
.extern __phidias_data_end
.extern __phidias_bss_start
.extern __phidias_bss_end
.extern __phidias_data_shared_start
.extern __phidias_data_shared_end

.extern	__phidias_end

#endif
