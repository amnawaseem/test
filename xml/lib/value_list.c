#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <schism/xml42.h>

const char *list_to_string(uint32_t *list, uint32_t length) {
	char *out_str;
	uint32_t out_len, out_mem;
	uint32_t i;

	out_len = 0;
	out_str = malloc(128);
	out_mem = 128;

	out_str[0] = '[';
	out_len++;
	for (i = 0; i < length; i++) {
		int written = snprintf(out_str + out_len, out_mem - out_len, "%u,", list[i]);
		if (written < 0) {
			abort();
		} else if ((unsigned int)written > out_mem - out_len) {
			out_str[out_len] = '\0';
			out_str = realloc(out_str, out_mem + 128);
			out_mem += 128;
			--i;
			continue;
		}
		out_len += written;
	}
	out_str[out_len-1] = ']';

	return out_str;
}

uint32_t *string_to_list(const char *str, uint32_t *length) {
	uint32_t *ret;
	uint32_t mem_length;

	if (str[0] != '[')
		return NULL;

	mem_length = 8;
	*length = 0;
	ret = malloc(mem_length * sizeof(int));

	++str;
	while (*str != ']') {
		char *ep;
		uint32_t v;

		if (*str == ',') {
			++str;
			continue;
		}

		v = strtoul(str, &ep, 0);
		if (ep == str) {
			free(ret);
			return NULL;
		} else {
			str = ep;
			ret[(*length)++] = v;
		}
		if (*length == mem_length) {
			mem_length += 8;
			ret = realloc(ret, mem_length * sizeof(int));
		}
	}

	return ret;
}

// ---------------------------------------------------------------------------

int listV_find_element(uint32_t *list, uint32_t len, uint32_t val) {
	uint32_t i;

	for (i = 0; i < len; i++) {
		if (list[i] == val)
			return i;
	}

	return -1;
}

int listV_contains(uint32_t *list, uint32_t len, uint32_t val) {
	return (listV_find_element(list, len, val) >= 0) ? 1 : 0;
}

int listS_contains(const char *list_str, uint32_t val) {
	uint32_t *list, len;
	int ret;

	list = string_to_list(list_str, &len);

	ret = listV_contains(list, len, val);

	free(list);

	return ret;
}

int list_contains(xmlNodePtr node, const char *attr, uint32_t val) {
	return listS_contains(get_attribute(node, attr), val);
}
