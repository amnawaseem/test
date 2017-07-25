#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>
#include "pagetables.h"

int create_flat(struct scenario_context *sctx) {
  set_attribute_hexnumber(query_element(sctx->doc, "/scenario/hypervisor/address_space"), "base", 0);
  return 0;
}
