#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

void LZ11UnCompVram(const struct CompressedData* src, volatile void* dest);
void LZ11UnCompWram(const struct CompressedData* src, volatile void* dest);
bool LZ11UnCompSuspendable(struct suspended_decompression*);
void LZ11UnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
