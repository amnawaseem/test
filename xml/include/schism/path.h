#ifndef _SCHISM_PATH_H_
#define _SCHISM_PATH_H_

#include <schism/xml42.h>

const char *search_path(const char *exe, const char *path);
const char *determine_cross_prefix(struct scenario_context *sctx);

#endif
