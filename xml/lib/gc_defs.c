#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <schism/xml42.h>
#include <schism/gc.h>

static void emit_string(int fd, const char *str);
static void emit_all_defs(int fd, struct addendum *A, int with_data);

// ---------------------------------------------------------------------------

struct addendum *init_addendum(struct addendum_section *seclist, uint32_t count) {
	struct addendum *A = malloc(sizeof(struct addendum));

	A->sections = seclist;
	A->section_count = count;
	A->deflist_head = NULL;
	A->deflist_tail = NULL;

	A->preamble = NULL;
	A->post_decl = NULL;
	A->epilogue = NULL;

	A->symbols_head = NULL;

	return A;
}

struct definition *new_addendum_definition(struct addendum *A, const char *type, uint32_t sec) {
	struct definition *def;

	def = malloc(sizeof(struct definition));
	def->datatype = type;
	init_def_buffer(&def->identifier, 128);
	init_def_buffer(&def->initializer, 128);
	def->section_index = sec;
	def->next = NULL;

	if (A->deflist_head == NULL) {
		A->deflist_head = def;
	} else {
		A->deflist_tail->next = def;
	}

	A->deflist_tail = def;

	return def;
}

// ---------------------------------------------------------------------------

void init_def_buffer(struct def_buffer *dbf, uint32_t init_len) {
	dbf->buf = malloc(init_len);
	dbf->buf[0] = '\0';
	dbf->buf_len = 0;
	dbf->buf_alloc = init_len;
}

void def_buffer_printf(struct def_buffer *dbf, const char *format, ...) {
	va_list args;
	int len;

	va_start(args, format);
	len = vsnprintf(dbf->buf + dbf->buf_len, dbf->buf_alloc - dbf->buf_len, format, args);
	va_end(args);
	if (len < 0) abort();

	while ((uint32_t)len >= dbf->buf_alloc - dbf->buf_len) {
		dbf->buf_alloc += 128;
		dbf->buf = realloc(dbf->buf, dbf->buf_alloc);

		va_start(args, format);
		len = vsnprintf(dbf->buf + dbf->buf_len, dbf->buf_alloc - dbf->buf_len, format, args);
		va_end(args);
		if (len < 0) abort();
	}

	dbf->buf_len += len;
}

// ---------------------------------------------------------------------------

static void emit_string(int fd, const char *str) {
	int ret, off, len;

	if (str == NULL)
		return;

	len = strlen(str);
	off = 0;

	while (off < len) {
		ret = write(fd, str + off, len - off);
		if (ret <= 0)
			abort();
		off += ret;
	}
}

static void emit_all_defs(int fd, struct addendum *A, int with_data) {
	struct definition *def;

	for (def = A->deflist_head; def; def = def->next) {
		struct addendum_section *sec = A->sections + def->section_index;

		if (sec->flags & ADDENDUM_SECTION_FLAG_EXTERN)
			emit_string(fd, "extern ");

		emit_string(fd, sec->elf_section_macro);
		emit_string(fd, " ");
		emit_string(fd, def->datatype);

		if (sec->flags & ADDENDUM_SECTION_FLAG_RO)
			emit_string(fd, " const");

		emit_string(fd, " ");
		emit_string(fd, def->identifier.buf);

		if (with_data) {
			emit_string(fd, " = ");
			emit_string(fd, def->initializer.buf);
		}

		emit_string(fd, ";\n");
	}

	emit_string(fd, "\n");
}

const char * const pass_fmt_src[2] = { "%s/scenario_config.c", "%s/scenario_config_real.c" };
const char * const pass_fmt_obj[2] = { "%s/scenario_config.xo", "%s/scenario_config_real.xo" };

int write_addendum(const char *builddir, struct addendum *A, int pass) {
	char fn[256];
	int fd;
	uint32_t i;

	snprintf(fn, 256, pass_fmt_src[pass-1], builddir);
	fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "ERROR: could not create addendum file.\n");
		return 1;
	}

	emit_string(fd, A->preamble);
	emit_string(fd, "\n");

	for (i = 0; i < A->section_count; i++) {
		char sec_line[128];

		snprintf(sec_line, 128, "#define\t%s\t__attribute__((section(\"%s\")))\n",
				A->sections[i].elf_section_macro,
				A->sections[i].elf_section_name);
		emit_string(fd, sec_line);
	}
	emit_string(fd, "\n");

	emit_all_defs(fd, A, 0);

	emit_string(fd, A->post_decl);
	emit_string(fd, "\n");

	emit_all_defs(fd, A, 1);

	emit_string(fd, A->epilogue);

	close(fd);

	return 0;
}
