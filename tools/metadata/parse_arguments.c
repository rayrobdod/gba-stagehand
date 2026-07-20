#include "parse_arguments.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct Arguments parseArguments(int argc, char** argv) {
	struct Arguments retval = {0};

	int i = 0;
	while (i < argc) {
		if (false) {
		}
		else if (0 == strcmp("--out-object", argv[i])) {
			if (NULL != retval.outCreditsObject) {
				retval.errorMsg = "--out-object provided twice";
				break;
			}
			i++;
			if (i >= argc) {
				retval.errorMsg = "no argument to --out-object";
				break;
			}
			retval.outCreditsObject = argv[i];
		}
		else if (0 == strcmp("--out-header", argv[i])) {
			if (NULL != retval.outCreditsHeader) {
				retval.errorMsg = "--out-header provided twice";
				break;
			}
			i++;
			if (i >= argc) {
				retval.errorMsg = "no argument to --out-header";
				break;
			}
			retval.outCreditsHeader = argv[i];
		}
		else if (0 == strcmp("--out-text", argv[i])) {
			if (NULL != retval.outTextFile) {
				retval.errorMsg = "--out-text provided twice";
				break;
			}
			i++;
			if (i >= argc) {
				retval.errorMsg = "no argument to --out-text";
				break;
			}
			retval.outTextFile = argv[i];
		}
		else {
			retval.inPngFiles = reallocarray(retval.inPngFiles, retval.inPngFileCount + 1, sizeof(const char*));
			retval.inPngFiles[retval.inPngFileCount] = argv[i];
			retval.inPngFileCount++;
		}

		i++;
	}

	if (0 == retval.inPngFileCount) {
		retval.errorMsg = "At least one input file required";
	}

	return retval;
}
