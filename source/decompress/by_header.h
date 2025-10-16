#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

[[gnu::access(read_only, 1), gnu::access(write_only, 2)]]
void HeaderUnCompWram(const struct CompressedData* src, volatile void* dest);

[[gnu::access(read_only, 1), gnu::access(write_only, 2)]]
void HeaderUnCompVram(const struct CompressedData* src, volatile void* dest);

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void HeaderUnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);

/**
 * Returns `true` if the decompression is complete.
 * If reentered after completion, should do nothing and return true
 */
[[gnu::access(read_write, 1)]]
bool HeaderUnCompSuspendable(struct suspended_decompression*);
