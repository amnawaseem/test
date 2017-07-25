#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <schism/xml42.h>

extern const char *schemata[8];
extern const uint32_t schemata_lengths[8];

static const char *stage_suffixes[] = {
	"", "_expanded", "_reparented", "_measured",
	"_p_laidout", "_v_laidout", "_pagetables"
};

static void silent_errors(void *ctx, const char *fmt, ...) {
	struct scenario_context *sctx = ctx;
	unsigned int error_room;
	int written;
	va_list vargs;

	if (sctx->error_buffer == NULL) {
		sctx->error_buffer = malloc(4096);
		sctx->error_length = 0;
		error_room = 4096;
	} else {
		error_room = 4096 - (sctx->error_length & 4095);
	}

	va_start(vargs, fmt);

	written = vsnprintf(sctx->error_buffer + sctx->error_length, error_room, fmt, vargs);

	if (written < 0) {
		abort();
	} else if ((unsigned int)written >= error_room) {
		sctx->error_buffer = realloc(sctx->error_buffer, (sctx->error_length + 8191) & ~4095);
		error_room += 4096;
		written = snprintf(sctx->error_buffer + sctx->error_length, error_room, fmt, vargs);
		if ((unsigned int)written >= error_room)
			written = error_room - 1;
	}

	sctx->error_length += written;
}

static xmlDocPtr _load_xml(struct scenario_context *sctx, const char *filename) {
	xmlParserCtxtPtr xmlParser;
	xmlErrorPtr xmlErr;
	xmlDocPtr xml;

	xmlParser = xmlNewParserCtxt();
	xmlSetGenericErrorFunc(sctx, &silent_errors);

	xml = xmlCtxtReadFile(xmlParser, filename, "utf-8", XML_PARSE_NOBLANKS);

	xmlErr = xmlCtxtGetLastError(xmlParser);
	if (xmlErr)
		return NULL;

	return xml;
}

int load_auxiliary(struct scenario_context *sctx) {
	char fn[256];

	snprintf(fn, 256, "xmllib/%s_%s.xml", sctx->aux_category, sctx->aux_name);

	sctx->auxDoc = _load_xml(sctx, fn);

	if (sctx->auxDoc == NULL)
		return 1;

	return 0;
}

int load_scenario(struct scenario_context *sctx) {
	char fn[256];

	if (sctx->stage > STAGE_MAX)
		return 1;

	snprintf(fn, 256, "%s/scenario%s.xml", sctx->builddir, stage_suffixes[sctx->stage]);

	sctx->doc = _load_xml(sctx, fn);

	if (sctx->doc == NULL)
		return 1;

	return 0;
}

int validate_scenario(struct scenario_context *sctx) {
	xmlParserCtxtPtr xmlParser;
	xmlErrorPtr xmlErr;
	xmlSchemaParserCtxtPtr xmlSchemaParser;
	xmlSchemaPtr xmlSchema;
	xmlSchemaValidCtxtPtr xmlSchemaValidator;
	int valid;

	if (sctx->stage > STAGE_MAX)
		return 1;

	xmlParser = xmlNewParserCtxt();
	xmlSetGenericErrorFunc(sctx, &silent_errors);

	sctx->schemaDoc = xmlCtxtReadMemory(xmlParser, schemata[sctx->stage],
			schemata_lengths[sctx->stage], NULL, "utf-8", XML_PARSE_NOBLANKS);

	xmlErr = xmlCtxtGetLastError(xmlParser);
	if (xmlErr)
		return 1;

	xmlSchemaParser = xmlSchemaNewDocParserCtxt(sctx->schemaDoc);
	xmlSchema = xmlSchemaParse(xmlSchemaParser);

	xmlSchemaValidator = xmlSchemaNewValidCtxt(xmlSchema);
	valid = xmlSchemaValidateDoc(xmlSchemaValidator, sctx->doc);
	// printf("Valid? %d\n", valid);

	return valid;
}

int save_scenario(struct scenario_context *sctx) {
	char fn[256];
	xmlSaveCtxtPtr xmlSave;

	if (sctx->stage > STAGE_MAX)
		return 1;

	snprintf(fn, 256, "%s/scenario%s.xml", sctx->builddir, stage_suffixes[sctx->stage]);

	xmlSave = xmlSaveToFilename(fn, "utf-8", XML_SAVE_FORMAT);

	xmlSaveDoc(xmlSave, sctx->doc);
	xmlSaveFlush(xmlSave);
	xmlSaveClose(xmlSave);

	return 0;
}

#ifdef XML42_STANDALONE
int main() {
	struct scenario_context sctx;

	memset(&sctx, 0, sizeof(sctx));
	sctx.builddir = "../BUILD/x64vmx";
	sctx.stage = 6;

	load_scenario(&sctx);
	validate_scenario(&sctx);

	printf("Errors: [%s]\n", sctx.error_buffer);
}
#endif
