/**
 * Behold, it's the second incarnation of the XML (glue) library for the
 * fourth incarnation of the Static Hypervisor Configuration System.
 * That makes it libxml42 in my numbering scheme.
 *
 * Still just a clumsy assortment of wrappers.
 */

#ifndef _SCHISM_XML42_H_
#define _SCHISM_XML42_H_

#include <string.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlsave.h>

struct scenario_context {
	const char	*builddir;
	unsigned int	stage;
	xmlDocPtr	doc;

	const char	*aux_category;
	const char	*aux_name;
	xmlDocPtr	auxDoc;

	xmlDocPtr	schemaDoc;

	char		*error_buffer;
	unsigned int	error_length;
};

enum scenario_stage {
	SCENARIO_STAGE_PRISTINE = 0,
	SCENARIO_STAGE_EXPANDED = 1,
	SCENARIO_STAGE_REPARENTED = 2,
	SCENARIO_STAGE_MEASURED = 3,
	SCENARIO_STAGE_PHYSICAL_LAID_OUT = 4,
	SCENARIO_STAGE_VIRTUAL_LAID_OUT = 5,
	SCENARIO_STAGE_PAGETABLES = 6,
	STAGE_MAX = SCENARIO_STAGE_PAGETABLES
};

int load_scenario(struct scenario_context *sctx);
int validate_scenario(struct scenario_context *sctx);
int save_scenario(struct scenario_context *sctx);
int load_auxiliary(struct scenario_context *sctx);

void sort_children(struct scenario_context *sctx, xmlNodePtr);
xmlNodePtr find_child_by_attribute(xmlNodePtr parent, const char *nodeType,
                                const char *attrName, const char *attrVal);
const char *query_attribute(xmlDocPtr, const char *, const char *);
xmlNodePtr query_element(xmlDocPtr, const char *);
const char *get_attribute(xmlNodePtr, const char *);
void set_attribute(xmlNodePtr, const char *, const char *);
xmlNodePtr resolve(xmlDocPtr, const char *);

const char *list_to_string(uint32_t *list, uint32_t length);
uint32_t *string_to_list(const char *str, uint32_t *length);
int listV_find_element(uint32_t *list, uint32_t len, uint32_t val);
int listV_contains(uint32_t *list, uint32_t len, uint32_t val);
int listS_contains(const char *list_str, uint32_t val);
int list_contains(xmlNodePtr node, const char *attr, uint32_t val);

uint64_t dict_get_hexnumber(xmlNodePtr node, const char *attr, int32_t index);
uint64_t dictS_get_hexnumber(const char *dict, int32_t index);
void dict_append_hexnumber(xmlNodePtr node, const char *attr, int32_t index, uint64_t val);

int get_map_properties(struct scenario_context *sctx, xmlNodePtr map, uint32_t cpu_va, uint32_t cpu_pa,
                        uint64_t *pa, uint64_t *va, uint64_t *size);

#define	iterate_over_children(docptr, path, childName, childVar) \
	for (childVar = query_element(docptr, path)->children; childVar; childVar = childVar->next) { \
		if (strcmp((const char *)childVar->name, childName) != 0) continue;

#define	iterate_over_children_N(parent, childName, childVar) \
	for (childVar = parent->children; childVar; childVar = childVar->next) { \
		if (strcmp((const char *)childVar->name, childName) != 0) continue;

// ---------------------------------------------------------------------------

static inline xmlNodePtr get_child(xmlNodePtr parent, const char *nodeType) {
	return find_child_by_attribute(parent, nodeType, NULL, NULL);
}

static inline uint32_t count_children(xmlNodePtr parent, const char *nodeType) {
	xmlNodePtr child;
	uint32_t count = 0;

	iterate_over_children_N(parent, nodeType, child) //{
		++count;
	}

	return count;
}

static inline void set_attribute_number(xmlNodePtr node, const char *attr, uint64_t val) {
	char *valstr = malloc(32);
	sprintf(valstr, "%ld", val);
	set_attribute(node, attr, valstr);
}

static inline void set_attribute_hexnumber(xmlNodePtr node, const char *attr, uint64_t val) {
	char *valstr = malloc(32);
	sprintf(valstr, "0x%lx", val);
	set_attribute(node, attr, valstr);
}

static inline uint64_t query_attribute_number(xmlDocPtr doc, const char *path, const char *attr) {
	return strtoul(query_attribute(doc, path, attr), NULL, 0);
}

static inline uint64_t get_attribute_number(xmlNodePtr node, const char *attr) {
	return strtoul(get_attribute(node, attr), NULL, 0);
}

static inline xmlNodePtr find_hypervisor_map(struct scenario_context *sctx, const char *xref) {
	return find_child_by_attribute(
			query_element(sctx->doc, "/scenario/hypervisor/address_space"),
			"map", "xref", xref);
}

#endif
