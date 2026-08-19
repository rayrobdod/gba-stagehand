#include "decompress/lz11.h"

#include "decompress/type.h"
#include "utils/arraycount.h"

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void CommonUnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest) {
	*state = (struct suspended_decompression) {};
	state->src = src->data;
	state->dest = (volatile uint8_t*)dest;
	state->dest_end = dest + (src->size / sizeof(uint8_t));
	state->magic = src->magic;
}

[[gnu::alias("CommonUnCompSuspendableInit")]]
void IdentityUnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void LZ11UnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void LZ16UnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void LZ77UnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void RLUnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void RlZeroUnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void Frit8UnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void Frit16UnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);
