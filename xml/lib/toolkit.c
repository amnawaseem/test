#include <schism/xml42.h>

int get_map_properties(struct scenario_context *sctx, xmlNodePtr map, uint32_t cpu_va, uint32_t cpu_pa,
			uint64_t *pa, uint64_t *va, uint64_t *size) {
	const char *xref_str;
	xmlNodePtr xref;
	const char *addr_tmp_str;
	uint64_t pa_value, va_value, size_value;

	if ((get_attribute(map, "cpumap") != NULL) &&
			(!list_contains(map, "cpumap", cpu_va)))
		return 1;

	xref_str = get_attribute(map, "xref");
	if (xref_str == NULL)
		return 1;

	xref = resolve(sctx->doc, xref_str);
	if (xref == NULL)
		return 1;

	size_value = get_attribute_number(xref, "size");

	addr_tmp_str = get_attribute(xref, "base");
	if (addr_tmp_str == NULL) {
		pa_value = 0UL;
	} else if (addr_tmp_str[0] == '[') {
		pa_value = dictS_get_hexnumber(addr_tmp_str, cpu_pa);
	} else {
		pa_value = strtoul(addr_tmp_str, NULL, 0);
	}

	if (get_attribute(map, "offset") != NULL) {
		pa_value += get_attribute_number(map, "offset");
		size_value -= get_attribute_number(map, "offset");
	}

	if (get_attribute(map, "subsize") != NULL) {
		size_value = get_attribute_number(map, "subsize");
	}

	addr_tmp_str = get_attribute(map, "base");
	if (addr_tmp_str == NULL) {
		va_value = 0UL;
	} else if (addr_tmp_str[0] == '[') {
		va_value = dictS_get_hexnumber(addr_tmp_str, cpu_va);
	} else {
		va_value = strtoul(addr_tmp_str, NULL, 0);
	}

	if (pa)		*pa = pa_value;
	if (va)		*va = va_value;
	if (size)	*size = size_value;

	return 0;
}
