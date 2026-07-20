#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

void LZ16UnComp(const struct CompressedData* src, volatile void* dest);
bool LZ16UnCompSuspendable(struct suspended_decompression*);
void LZ16UnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
