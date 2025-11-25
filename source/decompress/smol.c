#include "decompress/smol.h"

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

struct decoding_tans_cell {
	uint8_t symbol;
	uint8_t bits;
	uint8_t next_state;
};

#define TANS_FREQUENCIES (64)
#define TANS_SYMBOLS (16)

[[gnu::access(write_only, 1), gnu::access(read_only, 2)]]
static void generate_decoding_tans_table(struct decoding_tans_cell retval[TANS_FREQUENCIES], const uint32_t* packed_frequencies) {
	uint8_t frequencies[TANS_SYMBOLS];
	frequencies[15] = 0;
	for (unsigned word_i = 0; word_i < 3; ++word_i, ++packed_frequencies) {
		uint32_t tmp = *packed_frequencies;

		for (unsigned subword_i = 0; subword_i < 5; subword_i++) {
			frequencies[word_i * 5 + subword_i] = (tmp >> (6*subword_i)) & 0x3F;
		}
		frequencies[15] |= ((tmp >> 30) & 0x3) << (2 * word_i);
	}

	unsigned table_index = 0;
	for (unsigned i = 0; i < TANS_SYMBOLS; ++i) {
		for (unsigned j = 0; j < frequencies[i]; ++j, ++table_index) {
			retval[table_index].symbol = i;
			retval[table_index].next_state = frequencies[i] + j;
			retval[table_index].bits = __builtin_clz(frequencies[i] + j) - __builtin_clz(TANS_FREQUENCIES);
			retval[table_index].next_state <<= retval[table_index].bits;
		}
	}
}

struct bitstream {
	const uint32_t* bits;
	unsigned offset;
};

static uint16_t parseTansBitstream(
			struct bitstream* bitstream,
			uint32_t* tansState,
			const struct decoding_tans_cell tans_table[TANS_FREQUENCIES]) {
	uint16_t retval = 0;
	#pragma GCC unroll 4
	for (unsigned nibble_i = 0; nibble_i < 4; ++nibble_i) {
		struct decoding_tans_cell cell = tans_table[*tansState];
		retval |= cell.symbol << (nibble_i * 4);
		*tansState = cell.next_state;
		unsigned offset = 0;
		for (unsigned k = 0; k < cell.bits; ++k) {
			offset |= ((((*bitstream->bits) >> bitstream->offset) & 1) << k);
			bitstream->offset += 1;
			if (bitstream->offset >= 32) {
				bitstream->offset -= 32;
				bitstream->bits += 1;
			}
		}
		*tansState |= offset;
		*tansState %= TANS_FREQUENCIES;
	}
	return retval;
}

void Smol1UnComp(const struct CompressedData* src, volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	//const uint32_t tansState = src->data[4] & 0x3F;
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

void Smol2UnComp(const struct CompressedData* src, volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	//const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	uint32_t tansState = src->data[4] & 0x3F;
	const uint32_t bitstreamSize = (src->data[4] >> 6) | (src->data[5] << 2) | ((src->data[6] & 0x7) << 10);
	const uint32_t lengthoffsetSize = (src->data[6] >> 3) | (src->data[7] << 5);

	volatile uint16_t* dest16 = (volatile uint16_t*) dest;

	struct decoding_tans_cell symbol_tans_table[TANS_FREQUENCIES];
	generate_decoding_tans_table(symbol_tans_table, (const uint32_t*) (src->data + 8));

	struct bitstream bitstream = {
		.bits = (const uint32_t*) (src->data + 8 + 12),
		.offset = 0,
	};

	const uint8_t* lenOffs = (src->data + 8 + 12 + 4 * bitstreamSize);
	const uint8_t* const lenOffs_end = lenOffs + lengthoffsetSize;

	while (lenOffs < lenOffs_end) {
		const unsigned length = parseVarint(&lenOffs);
		const unsigned offset = parseVarint(&lenOffs);

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				*dest16 = parseTansBitstream(&bitstream, &tansState, symbol_tans_table);
				++dest16;
			}
		} else {
			*dest16 = parseTansBitstream(&bitstream, &tansState, symbol_tans_table);
			++dest16;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}
}
