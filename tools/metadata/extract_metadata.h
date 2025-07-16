#ifndef EXTRACT_METADATA_FROM_PNG_H
#define EXTRACT_METADATA_FROM_PNG_H

#include <stdbool.h>
#include <stdlib.h>

struct metadata {
	char* file_name;
	char* title;
	char* author;
	char* author_url;
	char* retrieved_from;
	char* licensed_under;
};

struct metadata_result {
	char* error_message;
	struct metadata value;
};

struct metadata_result extract_metadata_from_png(
	const char* file_name);

static inline void free_metadata_fields(struct metadata metadata) {
	free(metadata.file_name);
	free(metadata.title);
	free(metadata.author);
	free(metadata.author_url);
	free(metadata.retrieved_from);
	free(metadata.licensed_under);
}

#endif        //  #ifndef EXTRACT_METADATA_FROM_PNG_H
