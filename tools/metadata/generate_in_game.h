#include "extract_metadata.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void generate_credits_header(
	const char* out_file_name);

void generate_credits_object(
	const char* out_file_name,
	size_t metadata_count,
	const struct metadata_and_filenames* metadatas);

#ifdef __cplusplus
}
#endif
