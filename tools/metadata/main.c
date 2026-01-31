#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "parse_arguments.h"
#include "extract_metadata.h"
#include "generate_in_game.h"
#include "generate_text_file.h"

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
	struct metadata_and_filenames* metadatas = calloc(args.inPngFileCount, sizeof(struct metadata_and_filenames));

	for (size_t i = 0; i < args.inPngFileCount; i++) {
		struct metadata_result result = extract_metadata_from_png(args.inPngFiles[i]);
		if (result.error_message) {
			printf("%s: %s\n", args.inPngFiles[i], result.error_message);
			return -1;
		} else {
			size_t j;
			for (j = 0; j < metadata_count; j++) {
				if (equal_metadata(&result.value, &metadatas[j].metadata)) {
					metadatas[j].filenames_count += 1;
					metadatas[j].filenames = reallocarray(metadatas[j].filenames, metadatas[j].filenames_count, sizeof(char*));
					metadatas[j].filenames[metadatas[j].filenames_count - 1] = args.inPngFiles[i];

					free_metadata_fields(result.value);
					break;
				}
			}
			if (j == metadata_count) {
				metadata_count += 1;
				metadatas[j].metadata = result.value;
				metadatas[j].filenames_count = 1;
				metadatas[j].filenames = calloc(1, sizeof(char*));
				metadatas[j].filenames[0] = args.inPngFiles[i];
			}
		}
	}

	if (NULL != args.outCreditsHeader) {
		generate_credits_header(args.outCreditsHeader);
	}
	if (NULL != args.outCreditsObject) {
		generate_credits_object(args.outCreditsObject, metadata_count, metadatas);
	}
	if (NULL != args.outTextFile) {
		generate_text_file(args.outTextFile, metadata_count, metadatas);
	}

	for (unsigned i = 0; i < args.inPngFileCount; i++) {
		free_metadata_fields(metadatas[i].metadata);
		free(metadatas[i].filenames);
	}
	free(metadatas);
	free(args.inPngFiles);

	return 0;
}
