#ifndef EXTRACT_METADATA_FROM_PNG_H
#define EXTRACT_METADATA_FROM_PNG_H

#include <stdbool.h>
#include <stdlib.h>

struct tasl {
	char* title;
	char* author;
	char* author_url;
	char* retrieved_from;
	char* licensed_under;
};

struct metadata {
	struct tasl tasl;
	struct tasl derived_from_tasl;
};

struct metadata_and_filenames {
	struct metadata metadata;
	size_t filenames_count;
	const char** filenames;
};

struct metadata_result {
	char* error_message;
	struct metadata value;
};

struct metadata_result extract_metadata_from_png(
	const char* file_name);

void free_metadata_fields(struct metadata);

bool equal_metadata(const struct metadata*, const struct metadata*);

#endif        //  #ifndef EXTRACT_METADATA_FROM_PNG_H
