#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

void HuffUnCompVram(const struct CompressedData* src, volatile void* dest);
void HuffUnCompWram(const struct CompressedData* src, volatile void* dest);
bool HuffUnCompSuspendable(struct suspended_decompression*);

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void HuffUnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
