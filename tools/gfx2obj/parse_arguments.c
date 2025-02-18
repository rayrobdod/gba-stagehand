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
		const struct Flag* flag = template->flags;
		if (flag) {
			while (flag->key) {
				if (0 == strcmp(flag->key, argv[i])) {
					flag->setter(accumulator);
					goto next;
				}
				flag++;
			}
		}

		const struct Argument* argument = template->arguments;
		if (argument) {
			while (argument->key) {
				if (0 == strcmp(argument->key, argv[i])) {
					i++;
					if (i >= argc) {
						fprintf(stderr, "No value for final arg");
						exit(1);
					}
					argument->setter(accumulator, argv[i]);
					goto next;
				}
				argument++;
			}
		}

		if (0 == strcmp("--help", argv[i])) {
			flag = template->flags;
			if (flag) {
				while (flag->key) {
					printf("  %15s : \n", flag->key);
					flag++;
				}
			}
			argument = template->arguments;
			if (argument) {
				while (argument->key) {
					printf("  %15s : \n", argument->key);
					argument++;
				}
			}

			exit(0);
		}

		fprintf(stderr, "Unknown flag/arg: %s\n", argv[i]);
		exit(1);

		next:
		i++;
	}
}
