#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <schism/xml42.h>
#include <schism/gc.h>
#include <schism/path.h>

// --------------------------------------------------------------------------

static uint64_t previous_config_start_symbol;

static uint64_t _query_symbol_0(struct addendum *A, const char *symbol_name) {
  struct addendum_symbol *sym;

  for (sym = A->symbols_head; sym; sym = sym->next) {
    if (strcmp(sym->symbol, symbol_name) != 0)
      continue;

    return sym->address;
  }

  return (uint64_t)0;
}

uint64_t query_symbol(struct addendum *A, const char *symbol_name) {
  uint64_t addr = _query_symbol_0(A, symbol_name);

  return (addr - previous_config_start_symbol) + base_address;
}

// --------------------------------------------------------------------------

static void insert_symbol(struct addendum *A, struct addendum_symbol *sym) {
  struct addendum_symbol **symp;

  for (symp = &A->symbols_head; *symp; symp = &((*symp)->next)) {
    if ((*symp)->address > sym->address)
      break;
  }

  sym->next = *symp;
  *symp = sym;
}

extern char * const * environ;

int load_reflections(struct scenario_context *sctx, struct addendum *A, int pass) {
  int F;
  int fork_pipe[2];
  char fn[256];
  const char *ccprefix, *exe;
  unsigned int sym_count;
  char sym_buf[256];

  if (pass == 1)
    return 0;

  ccprefix = determine_cross_prefix(sctx);
  if (ccprefix == NULL) {
    fprintf(stderr, "ERROR: could not determine cross-compiler prefix.\n");
    return 1;
  }
  snprintf(fn, 256, "%snm", ccprefix);
  exe = search_path(fn, NULL);
  if (exe == NULL) {
    fprintf(stderr, "ERROR: cross-compiler toolchain does not offer 'nm' utility.\n");
    return 1;
  }

  snprintf(fn, 256, pass_fmt_obj[pass-1], sctx->builddir);

  pipe(fork_pipe);
  F = fork();

  if (F < 0) {
    return 1;
  } else if (F == 0) {
    char * const argv[3] = { (char *)exe, (char *)fn, NULL };

    close(fork_pipe[0]);
    close(0);
    dup2(fork_pipe[1], 1);
    close(2);

    execve(argv[0], argv, environ);
    exit(1);
  }

  close(fork_pipe[1]);

  memset(sym_buf, 0, 256);

  sym_count = 0;
  while (1) {
    struct addendum_symbol *sym;
    char *lb, *nl, *sp;
    int R = read(fork_pipe[0], sym_buf + strlen(sym_buf), 255 - strlen(sym_buf));

    if (R == 0)
      break;

    if (R < 0) {
      return 1;
    }

    for (lb = sym_buf, nl = strchr(sym_buf, '\n'); nl; nl = strchr(lb, '\n')) {
      *nl = 0;
      sp = strchr(lb, ' ');
      *sp = 0;
      *(sp+2) = 0;

      sym = malloc(sizeof(struct addendum_symbol));
      sym->address = strtoul(lb, NULL, 16);
      sym->symtype = *(sp+1);
      sym->symbol = strdup(sp+3);

      insert_symbol(A, sym);
      ++sym_count;
      ++nl;
      lb = nl;
    }

    memmove(sym_buf, lb, strlen(lb)+1);
    memset(sym_buf + strlen(sym_buf), 0, 256 - strlen(sym_buf));
  }

  close(fork_pipe[0]);

  if (strlen(sym_buf) > 0)
    return 1;

  previous_config_start_symbol = _query_symbol_0(A, "CONFIG_START");

  fprintf(stderr, "INFO: %d config reflection symbols loaded.\n", sym_count);
  return 0;
}
