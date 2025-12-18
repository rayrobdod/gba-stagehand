#include "decompress/by_header.h"

#include <stddef.h>
#include <stdint.h>
#include "decompress/frit.h"
#include "decompress/huff.h"
#include "decompress/identity.h"
#include "decompress/lz.h"
#include "decompress/lz11.h"
#include "decompress/rl.h"
#include "decompress/rlzero.h"
#include "decompress/smol.h"
#include "decompress/type.h"
#include "gba/bios.h"
#include "mgba.h"

#define CAT1(A, B) A##B
#define CAT(A, B) CAT1(A, B)
#define STR(A) #A

typedef void (*UnCompFn)(const struct CompressedData*, volatile void*);

typedef bool (*UnCompSuspendableFn)(struct suspended_decompression*);

typedef void (*UnCompSuspendableInitFn)(
	struct suspended_decompression*,
	const struct CompressedData*,
	volatile void*);

#define XRAM Vram
#include "decompress/by_header_template.h"
#undef XRAM

#define XRAM Wram
#include "decompress/by_header_template.h"
#undef XRAM

static UnCompSuspendableFn MagicToUnCompSuspendable(unsigned magic) {
	switch (magic) {
	case 0x00:
		return &IdentityUnCompSuspendable;
	case 0x10:
		return &LZ77UnCompSuspendable;
	case 0x11:
		return &LZ11UnCompSuspendable;
	case 0x24:
	case 0x28:
		return &HuffUnCompSuspendable;
	case 0x30:
		return &RLUnCompSuspendable;
	case 0x31:
		return &RlZeroUnCompSuspendable;
	case 0x41:
		return &Frit8UnCompSuspendable;
	case 0x42:
		return &Frit16UnCompSuspendable;
	case 0xF1:
		return &Smol1UnCompSuspendable;
	default:
		return NULL;
	}
}

bool HeaderUnCompSuspendable(struct suspended_decompression* state) {
	const uint8_t magic = state->magic;
	UnCompSuspendableFn fn = MagicToUnCompSuspendable(magic);

	if (fn) {
		return fn(state);
	} else {
		MgbaPrintf(MGBA_LOG_FATAL, "Unknown compression magic: %02x", magic);
		return true;
	}
}

static UnCompSuspendableInitFn MagicToUnCompSuspendableInit(unsigned magic) {
	switch (magic) {
	case 0x00:
		return &IdentityUnCompSuspendableInit;
	case 0x10:
		return &LZ77UnCompSuspendableInit;
	case 0x11:
		return &LZ11UnCompSuspendableInit;
	case 0x24:
	case 0x28:
		return &HuffUnCompSuspendableInit;
	case 0x30:
		return &RLUnCompSuspendableInit;
	case 0x31:
		return &RlZeroUnCompSuspendableInit;
	case 0x41:
		return &Frit8UnCompSuspendableInit;
	case 0x42:
		return &Frit16UnCompSuspendableInit;
	case 0xF1:
		return &Smol1UnCompSuspendableInit;
	default:
		return NULL;
	}
}

void HeaderUnCompSuspendableInit(
		struct suspended_decompression* state,
		const struct CompressedData* src,
		volatile void* dest) {
	const uint8_t magic = src->magic;
	UnCompSuspendableInitFn fn = MagicToUnCompSuspendableInit(magic);

	if (fn) {
		fn(state, src, dest);
	} else {
		MgbaPrintf(MGBA_LOG_FATAL, "Unknown compression magic: %02x", magic);
	}
}
