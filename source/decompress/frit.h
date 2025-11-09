struct CompressedData;

void Frit16UnComp(const struct CompressedData* src, volatile void* dest);
void Frit8UnCompVram(const struct CompressedData* src, volatile void* dest);
void Frit8UnCompWram(const struct CompressedData* src, volatile void* dest);

#include <stdbool.h>
struct suspended_decompression;

bool Frit16UnCompSuspendable(struct suspended_decompression*);
bool Frit8UnCompSuspendable(struct suspended_decompression*);

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void Frit16UnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void Frit8UnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
