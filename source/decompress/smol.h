#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

void Smol1UnComp(const struct CompressedData* src, volatile void* dest);
void Smol2UnComp(const struct CompressedData* src, volatile void* dest);
void Smol4UnComp(const struct CompressedData* src, volatile void* dest);

bool SmolUnCompSuspendable(struct suspended_decompression*);
void SmolUnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
