#include "generate_in_game.h"

#include <stdio.h>

void generate_credits_header(
	const char* out_file_name
) {
	FILE *fp = fopen(out_file_name, "w");
	fprintf(fp, "#ifndef RESOURCE_CREDITS_H\n");
	fprintf(fp, "#define RESOURCE_CREDITS_H\n");
	fprintf(fp, "\n");
	fprintf(fp, "struct resource_tasl {\n");
	fprintf(fp, "	char* title;\n");
	fprintf(fp, "	char* author;\n");
	fprintf(fp, "	char* author_url;\n");
	fprintf(fp, "	char* retrieved_from;\n");
	fprintf(fp, "	char* licensed_under;\n");
	fprintf(fp, "};\n");
	fprintf(fp, "struct resource_credit {\n");
	fprintf(fp, "	struct resource_tasl primary;\n");
	fprintf(fp, "	struct resource_tasl derived_from;\n");
	fprintf(fp, "};\n");
	fprintf(fp, "\n");
	fprintf(fp, "extern const struct resource_credit resource_credits[];\n");
	fprintf(fp, "extern const struct resource_credit resource_credits_end[0];\n");
	fprintf(fp, "\n");
	fprintf(fp, "#endif");
	fclose(fp);
}
