#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <schism/xml42.h>

uint64_t dict_get_hexnumber(xmlNodePtr node, const char *attr, int32_t index) {
	return dictS_get_hexnumber(get_attribute(node, attr), index);
}

uint64_t dictS_get_hexnumber(const char *dict, int32_t index) {
	const char *p;
	char search[16];

	snprintf(search, 16, "[%d]=", index);

	for (p = dict; p; p = strchr(p, ';')) {
		if (*p == ';')
			++p;

		if (strncmp(p, search, strlen(search)) != 0)
			continue;

		return strtoul(p + strlen(search), NULL, 0);
	}

	abort();
}

void dict_append_hexnumber(xmlNodePtr node, const char *attr, int32_t index, uint64_t val) {
	const char *dict;
	char *dict_update;

	dict = get_attribute(node, attr);

	dict_update = malloc(strlen(dict) + 32);
	strcpy(dict_update, dict);

	sprintf(dict_update + strlen(dict_update), "%s[%d]=0x%lx",
			(strlen(dict) == 0) ? "" : ";", index, val);

	set_attribute(node, attr, dict_update);
}
