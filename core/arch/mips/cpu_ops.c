#include <phidias.h>
#include <arch/cp0_regs.h>
#include <arch/asm/cp0_regs.h>

uint32_t top_of_stack = 0;

void cpu_dcache_flush(int do_clean, int do_invalidate) {
	/* flush the d-cache */
	/* TODO */
	(void)do_clean;
	(void)do_invalidate;
}

unsigned short getGuestID (void){
	return (mfc0(c0_guestctl1) & 0xff);
}

void setGuestID(unsigned int id){
	mtc0(c0_guestctl1, (mfc0(c0_guestctl1) & 0xFFFFFF00) | (id & 0xFF));
}

unsigned short getRootGuestID ( void ){
	return ((mfc0(c0_guestctl1) >> 16) & 0xff);
}

void setRootGuestID(unsigned int id){
	mtc0(c0_guestctl1, (mfc0(c0_guestctl1) & 0xFF00FFFF) | ((id & 0xFF) << 16));
}

/* write to previous shadow set */
void wtpss(uint32_t val, uint32_t reg){
	switch (reg){
		case 0:
			printf("wtpss tried to write to $zero\r\n");
		break;
		case 1:
			asm volatile(".set noat \n\
				      wrpgpr %0, $1" : : "r" (val));
		break;
		case 2:
			asm volatile("wrpgpr %0, $2" : : "r" (val));
		break;
		case 3:
			asm volatile("wrpgpr %0, $3" : : "r" (val));
		break;
		case 4:
			asm volatile("wrpgpr %0, $4" : : "r" (val));
		break;
		case 5:
			asm volatile("wrpgpr %0, $5" : : "r" (val));
		break;
		case 6:
			asm volatile("wrpgpr %0, $6" : : "r" (val));
		break;
		case 7:
			asm volatile("wrpgpr %0, $7" : : "r" (val));
		break;
		case 8:
			asm volatile("wrpgpr %0, $8" : : "r" (val));
		break;
		case 9:
			asm volatile("wrpgpr %0, $9" : : "r" (val));
		break;
		case 10:
			asm volatile("wrpgpr %0, $10" : : "r" (val));
		break;
		case 11:
			asm volatile("wrpgpr %0, $11" : : "r" (val));
		break;
		case 12:
			asm volatile("wrpgpr %0, $12" : : "r" (val));
		break;
		case 13:
			asm volatile("wrpgpr %0, $13" : : "r" (val));
		break;
		case 14:
			asm volatile("wrpgpr %0, $14" : : "r" (val));
		break;
		case 15:
			asm volatile("wrpgpr %0, $15" : : "r" (val));
		break;
		case 16:
			asm volatile("wrpgpr %0, $16" : : "r" (val));
		break;
		case 17:
			asm volatile("wrpgpr %0, $17" : : "r" (val));
		break;
		case 18:
			asm volatile("wrpgpr %0, $18" : : "r" (val));
		break;
		case 19:
			asm volatile("wrpgpr %0, $19" : : "r" (val));
		break;
		case 20:
			asm volatile("wrpgpr %0, $20" : : "r" (val));
		break;
		case 21:
			asm volatile("wrpgpr %0, $21" : : "r" (val));
		break;
		case 22:
			asm volatile("wrpgpr %0, $22" : : "r" (val));
		break;
		case 23:
			asm volatile("wrpgpr %0, $23" : : "r" (val));
		break;
		case 24:
			asm volatile("wrpgpr %0, $24" : : "r" (val));
		break;
		case 25:
			asm volatile("wrpgpr %0, $25" : : "r" (val));
		break;
		case 26:
			asm volatile("wrpgpr %0, $26" : : "r" (val));
		break;
		case 27:
			asm volatile("wrpgpr %0, $27" : : "r" (val));
		break;
		case 28:
			asm volatile("wrpgpr %0, $28" : : "r" (val));
		break;
		case 29:
			asm volatile("wrpgpr %0, $29" : : "r" (val));
		break;
		case 30:
			asm volatile("wrpgpr %0, $30" : : "r" (val));
		break;
		case 31:
			asm volatile("wrpgpr %0, $31" : : "r" (val));
		break;
		default:
			printf("invalid reg in wtpss");
		break;
	}
}

/* read from previous shadow set */
uint32_t rfpss(uint32_t reg){
	uint32_t val;
	switch (reg){
		case 0:
			return 0;
		break;
		case 1:
			asm volatile("rdpgpr %0, $1" : "=r" (val));
		break;
		case 2:
			asm volatile("rdpgpr %0, $2" : "=r" (val));
		break;
		case 3:
			asm volatile("rdpgpr %0, $3" : "=r" (val));
		break;
		case 4:
			asm volatile("rdpgpr %0, $4" : "=r" (val));
		break;
		case 5:
			asm volatile("rdpgpr %0, $5" : "=r" (val));
		break;
		case 6:
			asm volatile("rdpgpr %0, $6" : "=r" (val));
		break;
		case 7:
			asm volatile("rdpgpr %0, $7" : "=r" (val));
		break;
		case 8:
			asm volatile("rdpgpr %0, $8" : "=r" (val));
		break;
		case 9:
			asm volatile("rdpgpr %0, $9" : "=r" (val));
		break;
		case 10:
			asm volatile("rdpgpr %0, $10" : "=r" (val));
		break;
		case 11:
			asm volatile("rdpgpr %0, $11" : "=r" (val));
		break;
		case 12:
			asm volatile("rdpgpr %0, $12" : "=r" (val));
		break;
		case 13:
			asm volatile("rdpgpr %0, $13" : "=r" (val));
		break;
		case 14:
			asm volatile("rdpgpr %0, $14" : "=r" (val));
		break;
		case 15:
			asm volatile("rdpgpr %0, $15" : "=r" (val));
		break;
		case 16:
			asm volatile("rdpgpr %0, $16" : "=r" (val));
		break;
		case 17:
			asm volatile("rdpgpr %0, $17" : "=r" (val));
		break;
		case 18:
			asm volatile("rdpgpr %0, $18" : "=r" (val));
		break;
		case 19:
			asm volatile("rdpgpr %0, $19" : "=r" (val));
		break;
		case 20:
			asm volatile("rdpgpr %0, $20" : "=r" (val));
		break;
		case 21:
			asm volatile("rdpgpr %0, $21" : "=r" (val));
		break;
		case 22:
			asm volatile("rdpgpr %0, $22" : "=r" (val));
		break;
		case 23:
			asm volatile("rdpgpr %0, $23" : "=r" (val));
		break;
		case 24:
			asm volatile("rdpgpr %0, $24" : "=r" (val));
		break;
		case 25:
			asm volatile("rdpgpr %0, $25" : "=r" (val));
		break;
		case 26:
			asm volatile("rdpgpr %0, $26" : "=r" (val));
		break;
		case 27:
			asm volatile("rdpgpr %0, $27" : "=r" (val));
		break;
		case 28:
			asm volatile("rdpgpr %0, $28" : "=r" (val));
		break;
		case 29:
			asm volatile("rdpgpr %0, $29" : "=r" (val));
		break;
		case 30:
			asm volatile("rdpgpr %0, $30" : "=r" (val));
		break;
		case 31:
			asm volatile("rdpgpr %0, $31" : "=r" (val));
		break;
		default:
			printf("invalid reg in rfpss\r\n");
			return 0;
		break;
	}
	return val;
}

void dummy( void ){
	return;
}

uint32_t rfss(uint32_t reg, uint32_t set){
	if(set > 7){
		printf("rfss: tried to read from shadow set %d, aborting\r\n", set);
		return 0;
	}
	uint32_t srsctl = mfc0(c0_srsctl);
	mtc0(c0_srsctl, (srsctl & 0xFFFFFE3F) | (set & 0x7) << 6);
	asm volatile("jalr.hb	%0" : : "r" (&dummy));
	uint32_t val = rfpss(reg);
	mtc0(c0_srsctl, srsctl);
	return val;
}

void wtss(uint32_t val, uint32_t reg, uint32_t set){
	if(set > 7){
		printf("wtss: tried to read from shadow set %d, aborting\r\n", set);
		return;
	}
	uint32_t srsctl = mfc0(c0_srsctl);
	mtc0(c0_srsctl, (srsctl & 0xFFFFFE3F) | (set & 0x7) << 6);
	asm volatile("jalr.hb	%0" : : "r" (&dummy));
	wtpss(val, reg);
	mtc0(c0_srsctl, srsctl);
}

