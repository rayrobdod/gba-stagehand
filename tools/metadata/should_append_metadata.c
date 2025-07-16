#include "should_append_metadata.h"

#include <stdio.h>
#include <string.h>
#include "extract_metadata.h"

#define arraycount(a) (sizeof(a) / sizeof(a[0]))

static const char* const skip_auhor_urls[] = {
	"https://rayrobdod.name/",
};

static bool nullable_strequal(char* a, char* b) {
	if (a == NULL && b == NULL) {
		return true;
	} else if (a == NULL || b == NULL) {
		return false;
	} else {
		return 0 == strcmp(a, b);
	}
}

bool should_append_metadata(
	const struct metadata* candidate,
	size_t metadata_count,
	const struct metadata* metadatas
) {
	if (candidate->author_url) {
		for (unsigned skip_i = 0; skip_i < arraycount(skip_auhor_urls); skip_i++) {
			if (0 == strcmp(skip_auhor_urls[skip_i], candidate->author_url)) {
				printf("%s: skipping due to author_url %s\n", candidate->file_name, skip_auhor_urls[skip_i]);
				return false;
			}
		}
	}

	for (unsigned i = 0; i < metadata_count; i++) {
		if (
			nullable_strequal(metadatas[i].title, candidate->title) &&
			nullable_strequal(metadatas[i].author, candidate->author) &&
			nullable_strequal(metadatas[i].author_url, candidate->author_url) &&
			nullable_strequal(metadatas[i].retrieved_from, candidate->retrieved_from) &&
			nullable_strequal(metadatas[i].licensed_under, candidate->licensed_under) &&
			true
		) {
			return false;
		}
	}

	return true;
}
