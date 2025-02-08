#include "parse_arguments.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void parseArguments(
	int argc, char** argv,
	void* accumulator,
	const struct ArgumentsAndFlags* template
) {
	int i = 0;
	while (i < argc) {
		for (int j = 0; j < template->flags_count; j++) {
			if (0 == strcmp(template->flags[j].key, argv[i])) {
				template->flags[j].setter(accumulator);
				goto next;
			}
		}
		for (int j = 0; j < template->args_count; j++) {
			if (0 == strcmp(template->arguments[j].key, argv[i])) {
				i++;
				if (i >= argc) {
					fprintf(stderr, "No value for final arg");
					exit(1);
				}
				template->arguments[j].setter(accumulator, argv[i]);
				goto next;
			}
		}

		if (0 == strcmp("--help", argv[i])) {
			for (int j = 0; j < template->flags_count; j++) {
				printf("  %15s : \n", template->flags[j].key);
			}
			for (int j = 0; j < template->args_count; j++) {
				printf("  %15s : \n", template->arguments[j].key);
			}

			exit(0);
		}

		next:
		i++;
	}
}
