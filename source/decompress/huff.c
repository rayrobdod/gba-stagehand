#include "decompress/huff.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "utils/arraycount.h"
#include "mgba.h"

bool HuffUnCompSuspendable(struct suspended_decompression* state) {
	unsigned datasize = state->magic & 0xF;
	unsigned datamask = (0xFFFFFFFF << (32 - datasize)) >> (32 - datasize);

	unsigned inIntraOffset = state->regs[0];
	uint32_t inWord = *((uint32_t*)state->src);

	while (state->dest < state->dest_end && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 2) || reg_lcd.VCOUNT >= DISPLAY_HEIGHT)) {
		uint32_t outWord = 0;
		for (unsigned outIntraOffset = 0; outIntraOffset < 8; outIntraOffset += datasize) {
			const uint8_t* tree = state->src_start;
			while (true) {
				uint8_t treeValue = *tree;

				unsigned branch = (inWord >> (31 - inIntraOffset)) & 1;

				inIntraOffset++;
				if (inIntraOffset >= 32) {
					state->src += 4;
					inWord = *((uint32_t*)state->src);
					inIntraOffset = 0;
				}

				tree = (const uint8_t*) ((((intptr_t) tree) & ~1) + (*tree & 0x3F) * 2 + 2 + branch);

				if (treeValue & (0x80 >> branch))
					break;
			}

			uint8_t treeValue = *tree;
			treeValue &= datamask;
			outWord |= (treeValue << outIntraOffset);
		}

		*(state->dest) = outWord;
		state->dest += 1;
	}

	state->regs[0] = inIntraOffset;
	return state->dest >= state->dest_end;
}

void HuffUnCompSuspendableInit(
		struct suspended_decompression* state,
		const struct CompressedData* src,
		volatile void* dest) {
	char tree_size = *(src->data);

	state->src = src->data + tree_size * 2 + 2;
	state->src_start = src->data + 1;
	state->dest = (volatile uint8_t*)dest;
	state->dest_end = dest + (src->size / sizeof(uint8_t));
	state->magic = src->magic;
	for (unsigned i = 0; i < arraycount(state->regs); i++)
		state->regs[i] = 0;
}
