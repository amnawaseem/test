#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <schism/xml42.h>

void sort_children(struct scenario_context *sctx, xmlNodePtr node) {
	const char *nodeType = NULL;
	xmlNodePtr xtype = NULL, xseq;
	xmlNodePtr xchild;
	unsigned int sortCount = 0, i;
	xmlNodePtr *sortees;

	if (sctx->schemaDoc == NULL)
		abort();

	// determine node type
	nodeType = (const char *)node->name;

	// traverse schema, lookup xsd:sequence specification for that type
	iterate_over_children(sctx->schemaDoc, "/schema", "complexType", xtype) //{
		const char *xname = get_attribute(xtype, "name");
		if (!xname || (xname[0] != 't') || (xname[1] != '_'))
			continue;
		if (strcmp(xname+2, nodeType) != 0)
			continue;

		break;
	}

	if (xtype == NULL)
		abort();

	xtype = xtype->children;
	if (xtype == NULL)
		abort();
	if (strcmp((const char *)xtype->name, "sequence") != 0)
		abort();

	// count len(node->children), allocate temp array
	for (xchild = node->children; xchild; xchild = xchild->next)
		sortCount++;

	// if there's nothing to sort, exit
	if (sortCount == 0)
		return;

	sortees = malloc(sortCount * sizeof(xmlNodePtr));

	// sort children into array (group by group)
	sortCount = 0;
	for (xseq = xtype->children; xseq; xseq = xseq->next) {
		const char *group = get_attribute(xseq, "name");
		iterate_over_children_N(node, group, xchild) //{
			sortees[sortCount++] = xchild;
		}
	}

	// rewrite child->next pointer chain
	node->children = sortees[0];
	node->last = sortees[sortCount-1];

	for (i = 0; i < sortCount; i++) {
		sortees[i]->prev = (i > 0) ? sortees[i-1] : NULL;
		sortees[i]->next = (i < sortCount-1) ? sortees[i+1] : NULL;
	}

	free(sortees);
}

xmlNodePtr find_child_by_attribute(xmlNodePtr parent, const char *nodeType,
				const char *attrName, const char *attrVal) {
	xmlNodePtr child;
	const char *attr;

	for (child = parent->children; child; child = child->next) {
		if (nodeType && (strcmp((const char *)child->name, nodeType) != 0))
			continue;

		if (attrName != NULL) {
			attr = get_attribute(child, attrName);

			if (!attr || (strcmp(attr, attrVal) != 0))
				continue;
		}

		return child;
	}

	return NULL;
}

const char *query_attribute(xmlDocPtr doc, const char *path, const char *attrName) {
	xmlNodePtr element = query_element(doc, path);

	return get_attribute(element, attrName);
}

static const char alphachars[] = "abcdefghijklmnopqrstuvwxyz_ABCDEFGHIJKLMNOPQRSTUVWXYZ";

xmlNodePtr query_element(xmlDocPtr doc, const char *path) {
	const char *componentStart, *componentEnd, *nextComponent;
	uint32_t array_index, i;
	xmlNodePtr parent = NULL;
	xmlNodePtr curNode;

	componentStart = path;

	while (*componentStart) {
		if (*componentStart == '/') {
			componentStart++;
			continue;
		}

		nextComponent = \
		componentEnd = componentStart + strspn(componentStart, alphachars);
		if (*componentEnd == '[') {
			array_index = strtoul(componentEnd+1, (char **)&nextComponent, 10);
			if (*nextComponent != ']') {
				fprintf(stderr, "ERROR: illegal path specification: %s\n", path);
				return NULL;
			}
			nextComponent++;
		} else if ((*componentEnd == '/') || (*componentEnd == '\0')) {
			array_index = 0;
		} else {
			fprintf(stderr, "ERROR: illegal path specification: %s\n", path);
			return NULL;
		}

		i = 0;
		for (curNode = parent ? parent->children : xmlDocGetRootElement(doc);
				curNode; curNode = curNode->next) {

			const char *nodename = (const char *)curNode->name;

			if (strlen(nodename) != (unsigned int)(componentEnd - componentStart))
				continue;

			if (strncmp(nodename, componentStart, componentEnd - componentStart) != 0)
				continue;

			if (i != array_index) {
				++i;
				continue;
			}

			break;
		}

		if (curNode == NULL) {
			fprintf(stderr, "ERROR: path specification does not match anything: %s\n", path);
			return NULL;
		}

		parent = curNode;
		componentStart = nextComponent;
	}

	return curNode;
}

const char *get_attribute(xmlNodePtr node, const char *attrName) {
	return (const char *)xmlGetProp(node, (xmlChar *)attrName);
#if 0
	xmlAttrPtr attribute;

	for (attribute = node->properties; attribute; attribute = attribute->next) {
		if (strcmp((const char *)attribute->name, attrName) == 0) {
			return (const char *)attribute->children->content;
		}
	}

	fprintf(stderr, "ERROR: no such attribute for %s: %s\n", node->name, attrName);
	return NULL;
#endif
}

void set_attribute(xmlNodePtr node, const char *attrName, const char *attrValue) {
	xmlSetProp(node, (xmlChar *)attrName, (xmlChar *)attrValue);
}

static xmlNodePtr _resolve(xmlNodePtr node, const char *id) {
	xmlNodePtr elem, result;
	const char *id_attr;

	id_attr = get_attribute(node, "id");
	if (id_attr && (strcmp(id_attr, id) == 0)) {
		return node;
	}

	for (elem = node->children; elem; elem = elem->next) {
		result = _resolve(elem, id);
		if (result)
			return result;
	}

	return NULL;
}

xmlNodePtr resolve(xmlDocPtr doc, const char *id) {
	xmlNodePtr rootNode = xmlDocGetRootElement(doc);

	return _resolve(rootNode, id);
}
