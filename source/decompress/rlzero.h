/**

header:
	bits 0-7: the magic number 0x31
	bits 8-31: uncompressed length. Must be multiple of 2
frame:
	byte 0: Number of `0` u16s to write
	byte 1: Number of u16s to copy
	n*2 bytes: the u16s to copy

*/

#include <stdbool.h>

struct CompressedData;

void RlZeroUnComp(const struct CompressedData* src, volatile void* dest);

struct suspended_decompression;

bool RlZeroUnCompSuspendable(struct suspended_decompression*);

[[gnu::access(write_only, 1), gnu::access(read_only, 2), gnu::access(write_only, 3)]]
void RlZeroUnCompSuspendableInit(
	struct suspended_decompression*,
	const struct CompressedData* src,
	volatile void* dest);
