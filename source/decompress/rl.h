#include <stdbool.h>

struct CompressedData;

void RLUnCompVram(const struct CompressedData* src, volatile void* dest);
void RLUnCompWram(const struct CompressedData* src, volatile void* dest);

struct suspended_decompression;

bool RLUnCompSuspendable(struct suspended_decompression*);

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void RLUnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
