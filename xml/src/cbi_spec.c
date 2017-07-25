#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>

extern char * const *environ;

int main(int an, char **ac) {
  struct scenario_context sctx;
  xmlNodePtr scenarioNode;
  const char *cbiAttr;
  char fn[256];
  int i;

  if (an < 2) {
    fprintf(stderr, "ERROR: need build directory argument.\n");
    return 1;
  }

  sctx.builddir = ac[1];
  sctx.stage = CBI_FORWARDER_STAGE;

  if (load_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not load scenario: %s\n", sctx.error_buffer);
    return 1;
  }
  if (validate_scenario(&sctx) != 0) {
    fprintf(stderr, "ERROR: could not validate scenario: %s\n", sctx.error_buffer);
    return 1;
  }

  scenarioNode = query_element(sctx.doc, "/scenario");
  cbiAttr = get_attribute(scenarioNode, "cbi");

  snprintf(fn, 256, "./%s_%s", CBI_FORWARDER_NAME, cbiAttr);

  printf("%s", fn);
  for (i = 1; i < an; i++)
    printf(" %s", ac[i]);
  printf("\n");

  (void)execve(fn, ac, environ);
  fprintf(stderr, "ERROR: could not execute CBI-specific utility.\n");
  return 1;
}
