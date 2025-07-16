#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "parse_arguments.h"
#include "extract_metadata.h"
#include "generate_credits.h"
#include "should_append_metadata.h"


int main(int argc, char* argv[]) {
	if (argc < 1) {
		printf("TODO \n");
		return 0;
	}

	struct Arguments args = parseArguments(argc - 1, argv + 1);

	if (args.errorMsg) {
		printf("%s\n", args.errorMsg);
		return -1;
	}

	unsigned metadata_count = 0;
	struct metadata* metadatas = calloc(args.inPngFileCount, sizeof(struct metadata));

	for (size_t i = 0; i < args.inPngFileCount; i++) {
		struct metadata_result result = extract_metadata_from_png(args.inPngFiles[i]);
		if (result.error_message) {
			printf("%s: %s\n", args.inPngFiles[i], result.error_message);
			return -1;
		} else if (should_append_metadata(&result.value, metadata_count, metadatas)) {
			metadatas[metadata_count++] = result.value;
		} else {
			free_metadata_fields(result.value);
		}
	}

	if (NULL != args.outCreditsHeader) {
		generate_credits_header(args.outCreditsHeader);
	}
	if (NULL != args.outCreditsObject) {
		generate_credits_object(args.outCreditsObject, metadata_count, metadatas);
	}

	for (int i = 0; i < args.inPngFileCount; i++) {
		free_metadata_fields(metadatas[i]);
	}
	free(metadatas);
	free(args.inPngFiles);

	return 0;
}
