#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

void SmolUnComp(const struct CompressedData* src, volatile void* dest);
bool SmolUnCompSuspendable(struct suspended_decompression*);
void SmolUnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
