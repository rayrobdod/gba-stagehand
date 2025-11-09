#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

void LZ77UnCompVram(const struct CompressedData* src, volatile void* dest);
void LZ77UnCompWram(const struct CompressedData* src, volatile void* dest);
bool LZ77UnCompSuspendable(struct suspended_decompression*);
void LZ77UnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
