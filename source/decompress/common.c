#include "decompress/lz11.h"

#include "decompress/type.h"
#include "utils/arraycount.h"

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void CommonUnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest) {
	state->src = src->data;
	state->src_start = src->data;
	state->dest = (volatile uint8_t*)dest;
	state->dest_end = dest + (src->size / sizeof(uint8_t));
	state->magic = src->magic;
	for (unsigned i = 0; i < arraycount(state->regs); i++)
		state->regs[i] = 0;
}

[[gnu::alias("CommonUnCompSuspendableInit")]]
void IdentityUnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void LZ11UnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);

[[gnu::alias("CommonUnCompSuspendableInit")]]
void LZ77UnCompSuspendableInit(struct suspended_decompression* state, const struct CompressedData* src, volatile void* dest);
