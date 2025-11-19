#include "decompress/lz11.h"

#include <stdint.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "mgba.h"

static inline unsigned parseVarint(const uint8_t** lenOffs) {
	static const unsigned CONTINUE_MASK = 0x80;
	static const unsigned DATA_MASK = 0x7F;
	static const unsigned DATA_SHIFT = 7;

	unsigned retval = **lenOffs;
	++(*lenOffs);
	if (retval & CONTINUE_MASK) {
		retval &= DATA_MASK;
		retval |= (**lenOffs) << DATA_SHIFT;
		++(*lenOffs);
	}
	return retval;
}

void SmolUnComp(const struct CompressedData* src, volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	//const uint32_t initialState = src->data[4] & 0x3F;
	//const uint32_t bitstreamSize = (src->data[4] >> 6) | (src->data[5] << 2) | ((src->data[6] & 0x7) << 10);
	const uint32_t lengthoffsetSize = (src->data[6] >> 3) | (src->data[7] << 5);

	volatile uint16_t* dest16 = (volatile uint16_t*) dest;

	const uint16_t* symbols = (const uint16_t*) (src->data + 8);
	const uint8_t* lenOffs = (src->data + 8 + 2 * symbolsSize);
	const uint8_t* const lenOffs_end = lenOffs + lengthoffsetSize;

	while (lenOffs < lenOffs_end) {
		const unsigned length = parseVarint(&lenOffs);
		const unsigned offset = parseVarint(&lenOffs);

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				*dest16 = *symbols;
				++dest16;
				++symbols;
			}
		} else {
			*dest16 = *symbols;
			++dest16;
			++symbols;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}
}
