#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <elf.h>
#include <schism/xml42.h>
#include <schism/elf.h>

extern char * const *environ;

// ---------------------------------------------------------------------------

static int write_core(int image_fd, struct scenario_context *sctx, const char *input_file) {
  int input_fd;
  Elf64_Phdr *phdrs;
  xmlNodePtr memreq;
  uint64_t memreq_size;

  phdrs = elf_to_phdrs(input_file, 4);
  if (phdrs == NULL) {
    fprintf(stderr, "ERROR: could not read program headers from hypervisor ELF.\n");
    return 1;
  }

  input_fd = open(input_file, O_RDONLY);
  if (input_fd < 0) {
    fprintf(stderr, "ERROR: could not open hypervisor ELF.\n");
    return 1;
  }

  if ((phdrs[0].p_memsz != phdrs[0].p_filesz) ||
	(phdrs[1].p_memsz != phdrs[1].p_filesz) ||
	(phdrs[2].p_memsz != phdrs[2].p_filesz)) {
    fprintf(stderr, "ERROR: ELF file size differs from memory size for PHDR[0-2].\n");
    return 1;
  }

  fprintf(stderr, "INFO: embedding microkernel image (%6lx/%6lx/%6lx/%6lx).\n",
		phdrs[0].p_memsz, phdrs[1].p_memsz, phdrs[2].p_memsz, phdrs[3].p_memsz);

  memreq = resolve(sctx->doc, "core_rx");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  lseek(input_fd, phdrs[0].p_offset, SEEK_SET);
  sendfile(image_fd, input_fd, NULL, memreq_size);

  memreq = resolve(sctx->doc, "core_r");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  lseek(input_fd, phdrs[1].p_offset, SEEK_SET);
  sendfile(image_fd, input_fd, NULL, memreq_size);

  memreq = resolve(sctx->doc, "core_rws");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  lseek(input_fd, phdrs[2].p_offset, SEEK_SET);
  sendfile(image_fd, input_fd, NULL, memreq_size);

  memreq = resolve(sctx->doc, "core_rw");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  lseek(input_fd, phdrs[3].p_offset, SEEK_SET);
  sendfile(image_fd, input_fd, NULL, memreq_size);

  close(input_fd);

  return 0;
}

// ---------------------------------------------------------------------------

static int write_config(int image_fd, struct scenario_context *sctx, const char *input_file) {
  int input_fd;
  Elf64_Phdr *phdrs;
  xmlNodePtr memreq;
  uint64_t memreq_size;

  phdrs = elf_to_phdrs(input_file, 3);
  if (phdrs == NULL) {
    fprintf(stderr, "ERROR: could not read program headers from config data ELF.\n");
    return 1;
  }

  input_fd = open(input_file, O_RDONLY);
  if (input_fd < 0) {
    fprintf(stderr, "ERROR: could not open hypervisor ELF.\n");
    return 1;
  }

  fprintf(stderr, "INFO: embedding configuration data (%6lx/%6lx/%6lx).\n",
		phdrs[0].p_memsz, phdrs[1].p_memsz, phdrs[2].p_memsz);

  memreq = resolve(sctx->doc, "config_r");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  lseek(input_fd, phdrs[0].p_offset, SEEK_SET);
  sendfile(image_fd, input_fd, NULL, memreq_size);

  memreq = resolve(sctx->doc, "config_rw");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  lseek(input_fd, phdrs[1].p_offset, SEEK_SET);
  sendfile(image_fd, input_fd, NULL, memreq_size);

  memreq = resolve(sctx->doc, "config_rws");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  lseek(input_fd, phdrs[2].p_offset, SEEK_SET);
  sendfile(image_fd, input_fd, NULL, memreq_size);

  close(input_fd);

  return 0;
}

// ---------------------------------------------------------------------------

static int write_pagetables(int image_fd, struct scenario_context *sctx, const char *input_file) {
  int input_fd;
  struct stat input_fs;
  xmlNodePtr memreq;
  uint64_t memreq_size;

  input_fd = open(input_file, O_RDONLY);
  if (input_fd < 0) {
    fprintf(stderr, "ERROR: could not open pagetable file.\n");
    return 1;
  }
  (void)fstat(input_fd, &input_fs);

  memreq = resolve(sctx->doc, "pagetables");
  memreq_size = strtoul(get_attribute(memreq, "size"), NULL, 0);
  fprintf(stderr, "INFO: embedding pagetable data (%12lx).\n", input_fs.st_size);
  sendfile(image_fd, input_fd, NULL, input_fs.st_size);
  lseek(image_fd, memreq_size - input_fs.st_size, SEEK_CUR);

  close(input_fd);

  return 0;
}

// ---------------------------------------------------------------------------

static int write_blob(int image_fd, struct scenario_context *sctx) {
  xmlNodePtr file;
  const char *file_href;
  struct stat filest;
  struct tm filemtime;
  char filemtimestr[64];
  int input_fd;
  uint64_t input_len;

  iterate_over_children(sctx->doc, "/scenario/files", "file", file) //{
    file_href = get_attribute(file, "href");
    input_fd = open(file_href, O_RDONLY);
    if (input_fd < 0) {
      fprintf(stderr, "ERROR: could not open data file \"%s\".\n", file_href);
      return 1;
    }
    fstat(input_fd, &filest);
    input_len = strtoul(get_attribute(file, "size"), NULL, 0);
    if (input_len != (uint64_t)filest.st_size) {
      fprintf(stderr, "ERROR: file size of \"%s\" changed!\n", file_href);
      return 1;
    }
    localtime_r((time_t *)&filest.st_mtime, &filemtime);
    strftime(filemtimestr, 64, "%Y-%m-%d %H:%M:%S", &filemtime);
    fprintf(stderr, "INFO: embedding (%s) (%12lx) \"%s\".\n",
		filemtimestr, input_len, file_href);
    sendfile(image_fd, input_fd, NULL, input_len);
    close(input_fd);
  }

  return 0;
}

// ---------------------------------------------------------------------------

static int build_uimage(struct scenario_context *sctx, const char *builddir) {
  char srcfile[256];
  char dstfile[256];
  const char *argv[17] = { "mkimage", "-a", NULL, "-e", NULL, "-n", NULL,
			"-A", NULL, "-C", "none", "-T", "kernel",
			"-d", srcfile, dstfile, NULL };
  int F, W;

  argv[2] = query_attribute(sctx->doc, "/scenario/hypervisor", "load_base");
  argv[4] = query_attribute(sctx->doc, "/scenario/hypervisor", "entry");
  argv[6] = query_attribute(sctx->doc, "/scenario", "cbi");
  argv[8] = query_attribute(sctx->doc, "/scenario/platform/arch", "id");

  snprintf(srcfile, 256, "%s/image", builddir);
  snprintf(dstfile, 256, "%s/uimage", builddir);

  F = fork();
  if (F > 0) {
    (void)wait(&W);
  } else if (F == 0) {
    close(1);
    execve("/usr/bin/mkimage", (char **)argv, environ);
    exit(1);
  } else {
    fprintf(stderr, "ERROR: fork() failed.\n");
    return 1;
  }

  return W;
}

static int build_efi_pe(struct scenario_context *sctx, const char *builddir) {
  (void)sctx; (void)builddir;

  return 1;
}

// ---------------------------------------------------------------------------

static int build_multiboot(struct scenario_context *sctx, const char *builddir) {
  uint32_t mb_hdr[8] = { 0x1badb002, 0x10001, -0x1badb002 -0x10001, 0, 0, 0, 0, 0 };
  char srcfile[128];
  char dstfile[128];
  int srcfd, dstfd;
  uint32_t src_size;
  uint64_t tmp;

  tmp = strtoul(query_attribute(sctx->doc, "/scenario/hypervisor", "load_base"), NULL, 0);
  mb_hdr[3] = \
  mb_hdr[4] = (uint32_t)tmp - 8*4;

  tmp = strtoul(query_attribute(sctx->doc, "/scenario/hypervisor", "entry"), NULL, 0);
  mb_hdr[7] = (uint32_t)tmp;

  sprintf(srcfile, "%s/image", builddir);
  sprintf(dstfile, "%s/mbimage", builddir);

  srcfd = open(srcfile, O_RDONLY);
  if (srcfd < 0) {
    fprintf(stderr, "ERROR: cannot open $(O)/image.\n");
    return 1;
  }
  dstfd = open(dstfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dstfd < 0) {
    fprintf(stderr, "ERROR: cannot open $(O)/mbimage.\n");
    return 1;
  }

  if (write(dstfd, mb_hdr, 8*4) != 8*4) {
    fprintf(stderr, "ERROR: could not write multiboot header.\n");
    return 1;
  }

  src_size = lseek(srcfd, 0, SEEK_END);
  (void)lseek(srcfd, 0, SEEK_SET);

  while (src_size) {
    int32_t r = sendfile(dstfd, srcfd, NULL, src_size);
    if (r <= 0) {
      fprintf(stderr, "ERROR: error while copying image data.\n");
      return 1;
    }
    src_size -= r;
  }

  close(srcfd);
  close(dstfd);

  return 0;
}

// ---------------------------------------------------------------------------
#if 0
static uint8_t checksum8(const char *buffer, uint32_t length) {
  uint8_t sum = 0;
  uint32_t i;

  for (i = 0; i < length; i++) {
    sum += (uint8_t)(buffer[i]);
  }

  return (~sum + 1);
}

static void emit_ihex_line(int fd, uint8_t outtype, uint16_t offset,
				const char *buffer, uint32_t size) {
  char line[80];
  uint32_t i;
  uint8_t csum = checksum8(buffer, size);

  sprintf(line, ":%02x%04x%02x", size, offset, outtype);
  for (i = 0; i < size; i++) {
    sprintf(line + 9 + i*2, "%02x", buffer[i]);
  }
  sprintf(line + 9 + size*2, "%02x\n", csum);

  write(fd, line, 12 + size*2);
}

static int build_mips_ihex(struct xmlnode *scene, const char *builddir, const char *elffile) {
  char srcfile[128];
  char dstfile[128];
  int srcfd, dstfd;
  char buf[32];
  int nread;
  uint32_t address_base, address_entry;
  uint16_t tmp16;
  struct xmlnode *hypervisor = get_child(scene, STRUCT_HYPERVISOR);

  address_base = (uint32_t)hypervisor->attrs[HYPERVISOR_ATTR_LOAD_BASE].value.number;
  address_entry = (uint32_t)hypervisor->attrs[HYPERVISOR_ATTR_ENTRY].value.number;

  /* addresses are most likely in kseg0 or kseg1; mask off the top bits to get
     physical addresses suitable for flashing */

  address_base &= ~0xe0000000;
  address_entry &= ~0xe0000000;

  sprintf(srcfile, "%s/image", builddir);
  sprintf(dstfile, "%s/image.hex", builddir);

  dstfd = open(dstfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

  // Step 1: generate platform-specific data (fuses, boot code etc.)
  (void)elffile;
  // @@@

  // Step 2: read in image, convert to IHEX
  // Step 2a: emit code 04 (extended base)
  tmp16 = htons(address_base >> 16);
  emit_ihex_line(dstfd, 0x04, 0x0000, (char *)&tmp16, 2);

  // Step 2b: emit data 00
  srcfd = open(srcfile, O_RDONLY);

  while ((nread = read(srcfd, buf, 32)) > 0) {
    emit_ihex_line(dstfd, 0x00, (address_base & 0xffff), buf, nread);

    address_base += nread;

    if ((address_base >> 16) != tmp16) {
      tmp16 = htons(address_base >> 16);
      emit_ihex_line(dstfd, 0x04, 0x0000, (char *)&tmp16, 2);
    }
  }

  // Step 2c: emit entry point 05
  address_entry = htonl(address_entry);
  emit_ihex_line(dstfd, 0x05, 0x0000, (char *)&address_entry, 4);

  close(srcfd);
  close(dstfd);

  return 0;
}

static int build_arch_ihex(struct xmlnode *scene, const char *builddir) {
  char elffile[128];
  const char *elf_arch;

  sprintf(elffile, "%s/%s.elf", builddir, scene->attrs[SCENARIO_ATTR_CBI].value.string);

  elf_arch = elf_machine_type(elffile);

  if (strcmp(elf_arch, "MIPS") == 0) {
    return build_mips_ihex(scene, builddir, elffile);
  } else {
    fprintf(stderr, "ERROR: unknown architecture for building IHEX.\n");
    return 1;
  }
}
#else
static int build_arch_ihex(struct scenario_context *sctx, const char *builddir) {
  return 1;
}
#endif
// ---------------------------------------------------------------------------

int main(int an, char **ac) {
  struct scenario_context sctx;
  const char *image_type;
  char fn[256];
  int fd;

  memset(&sctx, 0, sizeof(sctx));

  if (an != 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = SCENARIO_STAGE_PAGETABLES;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
    return 1;
  }

  // ---------------------------------------------------------------------------

  snprintf(fn, 256, "%s/image", ac[1]);
  fd = open(fn, O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (fd < 0) {
    fprintf(stderr, "ERROR: could not create image file.\n");
    return 1;
  }

  snprintf(fn, 256, "%s/%s.elf", ac[1], query_attribute(sctx.doc, "/scenario", "cbi"));
  if (write_core(fd, &sctx, fn)) {
    fprintf(stderr, "ERROR: could not write core sections.\n");
    return 1;
  }

  snprintf(fn, 256, "%s/scenario_config_real.xo", ac[1]);
  if (write_config(fd, &sctx, fn)) {
    fprintf(stderr, "ERROR: could not write config data.\n");
    return 1;
  }

  snprintf(fn, 256, "%s/pagetables", ac[1]);
  if (write_pagetables(fd, &sctx, fn)) {
    fprintf(stderr, "ERROR: could not write pagetables.\n");
    return 1;
  }

  if (write_blob(fd, &sctx)) {
    fprintf(stderr, "ERROR: could not write payload blob.\n");
    return 1;
  }

  close(fd);

  image_type = query_attribute(sctx.doc, "/scenario", "image");
  if (strcmp(image_type, "uboot") == 0) {
    return build_uimage(&sctx, ac[1]);
  } else if (strcmp(image_type, "efi-pe") == 0) {
    return build_efi_pe(&sctx, ac[1]);
  } else if (strcmp(image_type, "multiboot") == 0) {
    return build_multiboot(&sctx, ac[1]);
  } else if (strcmp(image_type, "ihex") == 0) {
    return build_arch_ihex(&sctx, ac[1]);
  } else if (strcmp(image_type, "raw") == 0) {
    return 0;
  } else {
    fprintf(stderr, "ERROR: unknown image type.\n");
    return 1;
  }

  return 0;
}
