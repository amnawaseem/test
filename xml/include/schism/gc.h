#ifndef _SCHISM_GC_H_
#define _SCHISM_GC_H_

#include <stdint.h>

struct addendum_section {
	const char	*elf_section_name;
	const char	*elf_section_macro;
	uint32_t	flags;
};

#define	ADDENDUM_SECTION_FLAG_RO	1
#define	ADDENDUM_SECTION_FLAG_EXTERN	2

struct def_buffer {
	char		*buf;
	uint32_t	buf_alloc;
	uint32_t	buf_len;
};

struct definition {
	const char		*datatype;
	struct def_buffer	identifier;
	struct def_buffer	initializer;
	uint32_t		section_index;

	struct definition	*next;
};

struct addendum_symbol {
	uint64_t		address;
	const char		*symbol;
	char			symtype;
	struct addendum_symbol	*next;
};

struct addendum {
	struct addendum_section	*sections;
	uint32_t		section_count;

	struct definition	*deflist_head;
	struct definition	*deflist_tail;

	const char		*preamble;
	const char		*post_decl;
	const char		*epilogue;

	struct addendum_symbol	*symbols_head;
};

extern const char * const pass_fmt_src[2];
extern const char * const pass_fmt_obj[2];

extern uint64_t base_address;

// addendum file I/O and data management

struct addendum *init_addendum(struct addendum_section *, uint32_t count);
struct definition *new_addendum_definition(struct addendum *A, const char *type, uint32_t sec);
void init_def_buffer(struct def_buffer *dbf, uint32_t init_len);
void def_buffer_printf(struct def_buffer *dbf, const char *format, ...);
int write_addendum(const char *builddir, struct addendum *A, int pass);

// addendum reflection

int load_reflections(struct scenario_context *sctx, struct addendum *A, int pass);
uint64_t query_symbol(struct addendum *A, const char *symbol);

#endif
