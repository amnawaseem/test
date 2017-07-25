#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <schism/xml42.h>

const char stages[] = "0ermpvf";

struct scenario_context sctx;

static int do_xml_query(char *);

int main(int an, char **ac) {
	int optx;

	sctx.stage = 6;

	while ((optx = getopt(an, ac, "o:s:")) > 0) {
		switch (optx) {
		case 'o':
			sctx.builddir = optarg;
			break;
		case 's':
			if (strchr(stages, optarg[0]) == NULL) {
				fprintf(stderr, "Invalid chosen stage.\n");
				return 1;
			} else {
				sctx.stage = strchr(stages, optarg[0]) - stages;
			}
			break;
		default:
			fprintf(stderr, "Unknown option.\n");
			return 1;
		}
	}

	if (load_scenario(&sctx) != 0) {
		fprintf(stderr, "ERROR: could not load scenario: %s.\n", sctx.error_buffer);
		return 1;
	}
	if (validate_scenario(&sctx) != 0) {
		fprintf(stderr, "ERROR: could not validate scenario: %s.\n", sctx.error_buffer);
		return 1;
	}

	if (ac[optind] == NULL) {
		fprintf(stderr, "No XML path supplied.\n");
		return 1;
	}

	return do_xml_query(ac[optind]);
}

/**
 * Dump the requested piece of information from the XML tree.
 * Supported query strings:
 */
static int do_xml_query(char *str) {
	char *query, *subquery;
	xmlNodePtr node;
	const char *attr;

	query = strdup(str);
	subquery = strchr(query, ':');

	if (subquery) {
		*subquery = '\0';
		subquery++;
	}

	node = query_element(sctx.doc, query);

	if (node == NULL) {
		fprintf(stderr, "No such node: %s\n", query);
		return 1;
	}

	if (subquery == NULL) {
		return 0;
	}

	if (subquery[0] == '#') {
		printf("%d\n", count_children(node, subquery+1));
		return 0;
	}

	attr = get_attribute(node, subquery);

	if (attr == NULL) {
		fprintf(stderr, "Node %s has no such attribute: %s\n", query, subquery);
		return 1;
	}

	printf("%s\n", attr);

	return 0;
}
