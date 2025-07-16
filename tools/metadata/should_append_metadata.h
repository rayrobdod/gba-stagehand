#include <stdbool.h>
#include <stddef.h>

struct metadata;

bool should_append_metadata(
	const struct metadata* candidate,
	size_t metadata_count,
	const struct metadata* metadatas);
