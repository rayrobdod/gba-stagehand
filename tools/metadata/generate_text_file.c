#include "generate_text_file.h"

#include <stdio.h>

void generate_text_file(
	const char* out_file_name,
	size_t metadatas_count,
	const struct metadata_and_filenames* metadatas
) {
	FILE *fp = fopen(out_file_name, "w");
	for (size_t i = 0; i < metadatas_count; i++) {
		const struct metadata_and_filenames* metadata = &metadatas[i];

		fprintf(fp, "======\n");
		for (size_t j = 0; j < metadata->filenames_count; j++) {
			fprintf(fp, "* %s\n", metadata->filenames[j]);
		}
		fprintf(fp, "\n");
		fprintf(fp, "\"%s\" <%s>\nBy %s <%s>\n%s\n",
			metadata->metadata.tasl.title, metadata->metadata.tasl.retrieved_from,
			metadata->metadata.tasl.author, metadata->metadata.tasl.author_url,
			metadata->metadata.tasl.licensed_under);
		fprintf(fp, "\n");
		if (
			metadata->metadata.derived_from_tasl.title ||
			metadata->metadata.derived_from_tasl.retrieved_from ||
			metadata->metadata.derived_from_tasl.author ||
			metadata->metadata.derived_from_tasl.author_url ||
			metadata->metadata.derived_from_tasl.licensed_under ||
			false
		) {
			fprintf(fp, "Derived from:\n\"%s\" <%s>\nBy %s <%s>\n%s\n",
				metadata->metadata.derived_from_tasl.title, metadata->metadata.derived_from_tasl.retrieved_from,
				metadata->metadata.derived_from_tasl.author, metadata->metadata.derived_from_tasl.author_url,
				metadata->metadata.derived_from_tasl.licensed_under);
			fprintf(fp, "\n");
		}
	}
	fclose(fp);
}
