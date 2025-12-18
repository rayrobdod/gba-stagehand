#include <stdbool.h>

struct CompressedData;
struct suspended_decompression;

void Smol1UnComp(const struct CompressedData* src, volatile void* dest);
void Smol2UnComp(const struct CompressedData* src, volatile void* dest);
void Smol3UnComp(const struct CompressedData* src, volatile void* dest);
void Smol4UnComp(const struct CompressedData* src, volatile void* dest);
void Smol5UnComp(const struct CompressedData* src, volatile void* dest);
void Smol6UnComp(const struct CompressedData* src, volatile void* dest);
void Smol8UnComp(const struct CompressedData* src, volatile void* dest);

bool Smol1UnCompSuspendable(struct suspended_decompression*);
void Smol1UnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
