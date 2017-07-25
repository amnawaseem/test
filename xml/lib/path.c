#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <schism/xml42.h>
#include <schism/range.h>

#define		DIRSEP		'/'
#define		PATHSEP		':'
#define		PATHSEP_STR	":"

const char *search_path(const char *exe, const char *path) {
	char fn[256];
	const char *path_elem_start;
	unsigned int path_elem_length;

	if (path == NULL) {
		path = getenv("PATH");
	}

	if (path == NULL) {
		path = "/bin:/usr/bin";
	}

	for (path_elem_start = path;
			*path_elem_start != '\0';
			path_elem_start += path_elem_length) {

		path_elem_length = strcspn(path_elem_start, PATHSEP_STR);

		snprintf(fn, 256, "%.*s%c%s",
				path_elem_length,
				path_elem_start,
				DIRSEP,
				exe);

		if (access(fn, X_OK) == 0) {
			char *hit = strdup(fn);
			return hit;
		}

		if (path_elem_start[path_elem_length] == PATHSEP)
			path_elem_length++;
	}

	return NULL;
}

// ---------------------------------------------------------------------------

struct arch_to_cross {
	const char *arch;
	const char *prefix;
};

static const struct arch_to_cross crosslist[] = {
	{ "arm64",	"aarch64-unknown-elf-" },
	{ "arm64",	"aarch64-unknown-linux-gnu-" },
	{ "arm",	"arm-unknown-eabi-" },
	{ "arm",	"arm-none-eabi-" },
	{ "mips",	"mipsel-unknown-elf-" },
	{ "mips",	"mips-unknown-elf-" },
	{ "x86",	"x86_64-unknown-elf-" },
	{ NULL,		NULL }
};

static const char *try_ccprefix(const char *prefix) {
	char exe[256];

	snprintf(exe, 256, "%sgcc", prefix);

	return search_path(exe, NULL);
}

const char *determine_cross_prefix(struct scenario_context *sctx) {
	const char *ccprefix_conf;
	const char *arch;
	const char *result;
	unsigned int i;

	ccprefix_conf = query_attribute(sctx->doc, "/scenario/hypervisor", "ccprefix");

	if (ccprefix_conf) {
		result = try_ccprefix(ccprefix_conf);
		if (result) {
			free((char *)result);
			return ccprefix_conf;
		} else {
			return NULL;
		}
	}

	arch = query_attribute(sctx->doc, "/scenario/platform", "arch");
	if (arch == NULL) {
		arch = query_attribute(sctx->doc, "/scenario/platform/arch", "id");
	}

	for (i = 0; i < ARRAYLEN(crosslist); i++) {
		if (strcmp(crosslist[i].arch, arch) != 0)
			continue;

		result = try_ccprefix(crosslist[i].prefix);
		if (result != NULL) {
			free((char *)result);
			return crosslist[i].prefix;
		}
	}

	return NULL;
}
