#ifndef	_PHIDIAS_MULTIPLEXER_H_
#define	_PHIDIAS_MULTIPLEXER_H_

#ifdef FEATURE_MULTIPLEXER

void mux_out_char(uint8_t val,uint32_t id);
void mux_in_char(uint8_t ch);
void mux_out_string(const char* string);

#endif



#endif

