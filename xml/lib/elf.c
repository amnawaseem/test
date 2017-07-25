#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>

#define	__elfclass(eh)		(((char *)eh)[EI_CLASS])

static void *_elf_read_ph(const char *file, void *eh, unsigned int *count) {
	int f;
	void *ph;
	uint64_t ph_off, ph_entsize, ph_count;

	if (__elfclass(eh) == ELFCLASS32) {
		ph_off = ((Elf32_Ehdr *)eh)->e_phoff;
		ph_entsize = ((Elf32_Ehdr *)eh)->e_phentsize;
		ph_count = ((Elf32_Ehdr *)eh)->e_phnum;
	} else if (__elfclass(eh) == ELFCLASS64) {
		ph_off = ((Elf64_Ehdr *)eh)->e_phoff;
		ph_entsize = ((Elf64_Ehdr *)eh)->e_phentsize;
		ph_count = ((Elf64_Ehdr *)eh)->e_phnum;
	} else {
		fprintf(stderr, "ERROR: invalid ELF class.\n");
		return NULL;
	}

	f = open(file, O_RDONLY);
	if (f < 0) {
		fprintf(stderr, "ERROR: cannot open ELF file.\n");
		return NULL;
	}

	ph = malloc(ph_entsize * ph_count);
	lseek(f, ph_off, SEEK_SET);
	read(f, ph, ph_entsize * ph_count);
	close(f);

	*count = ph_count;
	return ph;
}

static void *_elf_read_eh(const char *file) {
	int f;
	void *eh;

	f = open(file, O_RDONLY);
	if (f < 0) {
		fprintf(stderr, "ERROR: cannot open ELF file.\n");
		return NULL;
	}

	eh = malloc(sizeof(Elf64_Ehdr));
	read(f, eh, EI_NIDENT);

	if (memcmp(eh, ELFMAG, SELFMAG) != 0) {
		fprintf(stderr, "ERROR: no ELF magic.\n");
		close(f);
		free(eh);
		return NULL;
	}

	if (__elfclass(eh) == ELFCLASS32) {
		read(f, eh+EI_NIDENT, sizeof(Elf32_Ehdr) - EI_NIDENT);
	} else if (__elfclass(eh) == ELFCLASS64) {
		read(f, eh+EI_NIDENT, sizeof(Elf64_Ehdr) - EI_NIDENT);
	} else {
		fprintf(stderr, "ERROR: invalid ELF class.\n");
		close(f);
		free(eh);
		return NULL;
	}

	close(f);
	return eh;
}

// ---------------------------------------------------------------------------

void *elf_to_phdrs(const char *file, unsigned int phdr_expect) {
	unsigned int phnum, i, j;
	void *eh, *ph, *phdr;

	eh = _elf_read_eh(file);
	if (eh == NULL) {
		return NULL;
	}

	ph = _elf_read_ph(file, eh, &phnum);
	if (ph == NULL) {
		free(eh);
		return NULL;
	}

	phdr = malloc(phnum * sizeof(Elf64_Phdr));

	for (i = 0, j = 0; i < phnum; i++) {
		Elf64_Phdr tmp_phdr;

		if (__elfclass(eh) == ELFCLASS32) {
			Elf32_Phdr *cur_phdr = ph + (i * sizeof(Elf32_Phdr));
			tmp_phdr.p_type = cur_phdr->p_type;
			tmp_phdr.p_flags = cur_phdr->p_flags;
			tmp_phdr.p_offset = cur_phdr->p_offset;
			tmp_phdr.p_vaddr = cur_phdr->p_vaddr;
			tmp_phdr.p_paddr = cur_phdr->p_paddr;
			tmp_phdr.p_filesz = cur_phdr->p_filesz;
			tmp_phdr.p_memsz = cur_phdr->p_memsz;
			tmp_phdr.p_align = cur_phdr->p_align;
		} else if (__elfclass(eh) == ELFCLASS64) {
			memcpy(&tmp_phdr, ph + (i * sizeof(Elf64_Phdr)), sizeof(Elf64_Phdr));
		}

		if (tmp_phdr.p_type != PT_LOAD) {
			continue;
		}

		if (j == phdr_expect) {
			fprintf(stderr, "ERROR: too many PT_LOAD PHDRs in ELF \"%s\", expected %d\n",
						file, phdr_expect);
			free(eh);
			free(ph);
			free(phdr);
			return NULL;
		}

		memcpy(phdr + j*sizeof(Elf64_Phdr), &tmp_phdr, sizeof(Elf64_Phdr));
		++j;
	}

	if (j != phdr_expect) {
		fprintf(stderr, "ERROR: too few PT_LOAD PHDRs in ELF \"%s\", expected %d, found %d\n",
					file, phdr_expect, j);
		free(phdr);
		phdr = NULL;
	}

	free(eh);
	free(ph);
	return phdr;
}

uint64_t elf_entry_point(const char *file) {
	void *eh;
	uint64_t entry;

	eh = _elf_read_eh(file);
	if (eh == NULL) {
		return ~0UL;
	}

	if (__elfclass(eh) == ELFCLASS32) {
		entry = ((Elf32_Ehdr *)eh)->e_entry;
	} else if (__elfclass(eh) == ELFCLASS64) {
		entry = ((Elf64_Ehdr *)eh)->e_entry;
	} else {
		fprintf(stderr, "ERROR: invalid ELF class.\n");
		entry = ~0UL;
	}

	free(eh);
	return entry;
}

const char *elf_machine_type(const char *file) {
	void *eh;
	uint32_t machtype;

	eh = _elf_read_eh(file);
	if (eh == NULL) {
		return NULL;
	}

	if (__elfclass(eh) == ELFCLASS32) {
		machtype = ((Elf32_Ehdr *)eh)->e_machine;
	} else if (__elfclass(eh) == ELFCLASS64) {
		machtype = ((Elf64_Ehdr *)eh)->e_machine;
	} else {
		fprintf(stderr, "ERROR: invalid ELF class.\n");
		machtype = ~0U;
	}

	free(eh);

	switch (machtype) {
	case EM_MIPS:		return "MIPS";
	default:		return "unknown";
	}
}
