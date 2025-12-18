#include "decompress/smol.h"

#include <stdint.h>
#include <stdlib.h>
#include "decompress/type.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "utils/arraycount.h"
#include "utils/comptime_for.h"
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

struct [[gnu::aligned(4)]] decoding_tans_cell {
	uint8_t symbol;
	uint8_t bits;
	uint8_t next_state;
	uint8_t mask;
};
_Static_assert(4 == sizeof(struct decoding_tans_cell));

#define TANS_FREQUENCIES (64)
#define TANS_SYMBOLS (16)

#define TANS_TABLE_TEMPLATE_ENTRY(j) \
	[j] = { \
		.symbol = 0, \
		.bits = __builtin_clz(j) - __builtin_clz(TANS_FREQUENCIES), \
		.next_state = (j << (__builtin_clz(j) - __builtin_clz(TANS_FREQUENCIES))) % TANS_FREQUENCIES, \
		.mask = (1 << (__builtin_clz(j) - __builtin_clz(TANS_FREQUENCIES))) - 1, \
	},

static const struct decoding_tans_cell decoding_tans_template[128] = {
	[0] = {0},
	FOR_1_TO_127(TANS_TABLE_TEMPLATE_ENTRY)
};

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
			retval[table_index] = decoding_tans_template[frequencies[i] + j];
			retval[table_index].symbol = i;
		}
	}
}

struct bitstream {
	const uint16_t* src;
	unsigned buffer;
	unsigned buffer_size;
};

static uint16_t parseTansBitstream_Nibble(
			struct bitstream* bitstream,
			uint32_t* tansState,
			const struct decoding_tans_cell tans_table[TANS_FREQUENCIES]) {
	if (__builtin_expect(bitstream->buffer_size < 16, 0)) {
		bitstream->buffer |= (*bitstream->src) << (bitstream->buffer_size);
		bitstream->buffer_size += 16;
		++(bitstream->src);
	}

	struct decoding_tans_cell cell = tans_table[*tansState];
	unsigned retval = cell.symbol;
	*tansState = cell.next_state;

	unsigned offset = bitstream->buffer & cell.mask;
	bitstream->buffer >>= cell.bits;
	bitstream->buffer_size -= cell.bits;

	*tansState |= offset;
	*tansState %= TANS_FREQUENCIES;
	return retval;
}

static uint16_t parseTansBitstream_DeltaNibble(
			struct bitstream* bitstream,
			uint32_t* tansState,
			uint32_t* previousNibble,
			const struct decoding_tans_cell tans_table[TANS_FREQUENCIES]) {
	unsigned delta = parseTansBitstream_Nibble(bitstream, tansState, tans_table);
	*previousNibble += delta;
	*previousNibble &= 0xF;
	return *previousNibble;
}

static uint16_t parseTansBitstream_u16(
			struct bitstream* bitstream,
			uint32_t* tansState,
			const struct decoding_tans_cell tans_table[TANS_FREQUENCIES]) {
	uint16_t retval = 0;
	for (unsigned nibble_i = 0; nibble_i < 4; ++nibble_i) {
		retval |= parseTansBitstream_Nibble(bitstream, tansState, tans_table) << (nibble_i * 4);
	}
	return retval;
}

static uint16_t parseTansBitstream_Deltau16(
			struct bitstream* bitstream,
			uint32_t* tansState,
			uint32_t* previousNibble,
			const struct decoding_tans_cell tans_table[TANS_FREQUENCIES]) {
	uint16_t retval = 0;
	for (unsigned nibble_i = 0; nibble_i < 4; ++nibble_i) {
		retval |= parseTansBitstream_DeltaNibble(bitstream, tansState, previousNibble, tans_table) << (nibble_i * 4);
	}
	return retval;
}

static uint16_t parseTansBitstream_Varint(
			struct bitstream* bitstream,
			uint32_t* tansState,
			uint32_t* lo_bytes_read,
			const struct decoding_tans_cell tans_table[TANS_FREQUENCIES]) {
	unsigned retval = 0;
	retval |= parseTansBitstream_Nibble(bitstream, tansState, tans_table);
	retval |= parseTansBitstream_Nibble(bitstream, tansState, tans_table) << 4;
	*lo_bytes_read += 1;
	if (retval & 0x80) {
		retval &= 0x7F;
		retval |= parseTansBitstream_Nibble(bitstream, tansState, tans_table) << 7;
		retval |= parseTansBitstream_Nibble(bitstream, tansState, tans_table) << 11;
		*lo_bytes_read += 1;
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
				*dest16++ = *symbols++;
			}
		} else {
			*dest16++ = *symbols++;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}
}

void Smol1UnCompSuspendableInit(
		struct suspended_decompression* state,
		const struct CompressedData* src,
		volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	//const uint32_t tansState = src->data[4] & 0x3F;
	//const uint32_t bitstreamSize = (src->data[4] >> 6) | (src->data[5] << 2) | ((src->data[6] & 0x7) << 10);
	const uint32_t lengthoffsetSize = (src->data[6] >> 3) | (src->data[7] << 5);

	state->src = src->data + 8 + 2 * symbolsSize;
	state->src_ptrs[0] = src->data + 8;
	state->src_ptrs[1] = src->data + 8 + 2 * symbolsSize + lengthoffsetSize;
	state->dest = (volatile uint8_t*)dest;
	state->dest_end = dest + (src->size / sizeof(uint8_t));
	state->magic = src->magic;
	for (unsigned i = 0; i < arraycount(state->regs); i++)
		state->regs[i] = 0;
}

bool Smol1UnCompSuspendable(struct suspended_decompression* state) {
	volatile uint16_t* dest16 = (volatile uint16_t*) state->dest;
	const uint16_t* symbols = (const uint16_t*) state->src_ptrs[0];
	const uint8_t* lenOffs = state->src;
	const uint8_t* const lenOffs_end = state->src_ptrs[1];

	while (lenOffs < lenOffs_end && (reg_lcd.VCOUNT < (DISPLAY_HEIGHT - 10) || reg_lcd.VCOUNT > DISPLAY_HEIGHT)) {
		const unsigned length = parseVarint(&lenOffs);
		const unsigned offset = parseVarint(&lenOffs);

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				*dest16++ = *symbols++;
			}
		} else {
			*dest16++ = *symbols++;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}

	state->dest = (volatile uint8_t*) dest16;
	state->src = lenOffs;
	state->src_ptrs[0] = (const uint8_t*) symbols;
	return lenOffs >= lenOffs_end;
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
		.src = (const uint16_t*) (src->data + 8 + 12),
		.buffer = 0,
		.buffer_size = 0,
	};

	const uint8_t* lenOffs = (src->data + 8 + 12 + 4 * bitstreamSize);
	const uint8_t* const lenOffs_end = lenOffs + lengthoffsetSize;

	while (lenOffs < lenOffs_end) {
		const unsigned length = parseVarint(&lenOffs);
		const unsigned offset = parseVarint(&lenOffs);

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				*dest16 = parseTansBitstream_u16(&bitstream, &tansState, symbol_tans_table);
				++dest16;
			}
		} else {
			*dest16 = parseTansBitstream_u16(&bitstream, &tansState, symbol_tans_table);
			++dest16;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}
}

void Smol3UnComp(const struct CompressedData* src, volatile void* dest) {
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
		.src = (const uint16_t*) (src->data + 8 + 12),
		.buffer = 0,
		.buffer_size = 0,
	};

	const uint8_t* lenOffs = (src->data + 8 + 12 + 4 * bitstreamSize);
	const uint8_t* const lenOffs_end = lenOffs + lengthoffsetSize;

	uint32_t previousNibble = 0;

	while (lenOffs < lenOffs_end) {
		const unsigned length = parseVarint(&lenOffs);
		const unsigned offset = parseVarint(&lenOffs);

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				*dest16 = parseTansBitstream_Deltau16(&bitstream, &tansState, &previousNibble, symbol_tans_table);
				++dest16;
			}
		} else {
			*dest16 = parseTansBitstream_Deltau16(&bitstream, &tansState, &previousNibble, symbol_tans_table);
			++dest16;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}
}

void Smol4UnComp(const struct CompressedData* src, volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	//const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	uint32_t tansState = src->data[4] & 0x3F;
	const uint32_t bitstreamSize = (src->data[4] >> 6) | (src->data[5] << 2) | ((src->data[6] & 0x7) << 10);
	const uint32_t lengthoffsetSize = (src->data[6] >> 3) | (src->data[7] << 5);

	volatile uint16_t* dest16 = (volatile uint16_t*) dest;

	struct decoding_tans_cell lo_tans_table[TANS_FREQUENCIES];
	generate_decoding_tans_table(lo_tans_table, (const uint32_t*) (src->data + 8));

	struct bitstream bitstream = {
		.src = (const uint16_t*) (src->data + 8 + 12),
		.buffer = 0,
		.buffer_size = 0,
	};

	const uint16_t* symbols = (const uint16_t*) (src->data + 8 + 12 + 4 * bitstreamSize);

	uint32_t lo_bytes_read = 0;

	while (lo_bytes_read < lengthoffsetSize) {
		const unsigned length = parseTansBitstream_Varint(&bitstream, &tansState, &lo_bytes_read, lo_tans_table);
		const unsigned offset = parseTansBitstream_Varint(&bitstream, &tansState, &lo_bytes_read, lo_tans_table);

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

void Smol5UnComp(const struct CompressedData* src, volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	//const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	uint32_t tansState = src->data[4] & 0x3F;
	//const uint32_t bitstreamSize = (src->data[4] >> 6) | (src->data[5] << 2) | ((src->data[6] & 0x7) << 10);
	const uint32_t lengthoffsetSize = (src->data[6] >> 3) | (src->data[7] << 5);

	volatile uint16_t* dest16 = (volatile uint16_t*) dest;

	struct decoding_tans_cell lo_tans_table[TANS_FREQUENCIES];
	generate_decoding_tans_table(lo_tans_table, (const uint32_t*) (src->data + 8));

	struct decoding_tans_cell symbol_tans_table[TANS_FREQUENCIES];
	generate_decoding_tans_table(symbol_tans_table, (const uint32_t*) (src->data + 8 + 12));

	struct bitstream bitstream = {
		.src = (const uint16_t*) (src->data + 8 + 12 + 12),
		.buffer = 0,
		.buffer_size = 0,
	};

	uint16_t* const instructions_start = malloc(2 * lengthoffsetSize);
	if (NULL == instructions_start) {
		MgbaPrintf(MGBA_LOG_FATAL, "Smol5UnComp: Out of memory");
		return;
	}

	uint32_t lo_bytes_read = 0;

	uint16_t* instructions = instructions_start;
	while (lo_bytes_read < lengthoffsetSize) {
		*instructions = parseTansBitstream_Varint(&bitstream, &tansState, &lo_bytes_read, lo_tans_table);
		instructions++;
		*instructions = parseTansBitstream_Varint(&bitstream, &tansState, &lo_bytes_read, lo_tans_table);
		instructions++;
	}
	uint16_t* const instructions_end = instructions;
	instructions = instructions_start;

	while (instructions < instructions_end) {
		const unsigned length = *instructions;
		instructions++;
		const unsigned offset = *instructions;
		instructions++;

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				*dest16 = parseTansBitstream_u16(&bitstream, &tansState, symbol_tans_table);
				++dest16;
			}
		} else {
			*dest16 = parseTansBitstream_u16(&bitstream, &tansState, symbol_tans_table);
			++dest16;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}

	free(instructions_start);
}

void Smol6UnComp(const struct CompressedData* src, volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	//const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	uint32_t tansState = src->data[4] & 0x3F;
	//const uint32_t bitstreamSize = (src->data[4] >> 6) | (src->data[5] << 2) | ((src->data[6] & 0x7) << 10);
	const uint32_t lengthoffsetSize = (src->data[6] >> 3) | (src->data[7] << 5);

	volatile uint16_t* dest16 = (volatile uint16_t*) dest;

	struct decoding_tans_cell lo_tans_table[TANS_FREQUENCIES];
	generate_decoding_tans_table(lo_tans_table, (const uint32_t*) (src->data + 8));

	struct decoding_tans_cell symbol_tans_table[TANS_FREQUENCIES];
	generate_decoding_tans_table(symbol_tans_table, (const uint32_t*) (src->data + 8 + 12));

	struct bitstream bitstream = {
		.src = (const uint16_t*) (src->data + 8 + 12 + 12),
		.buffer = 0,
		.buffer_size = 0,
	};

	uint16_t* const instructions_start = malloc(2 * lengthoffsetSize);
	if (NULL == instructions_start) {
		MgbaPrintf(MGBA_LOG_FATAL, "Smol6UnComp: Out of memory");
		return;
	}

	uint32_t lo_bytes_read = 0;

	uint16_t* instructions = instructions_start;
	while (lo_bytes_read < lengthoffsetSize) {
		*instructions = parseTansBitstream_Varint(&bitstream, &tansState, &lo_bytes_read, lo_tans_table);
		instructions++;
		*instructions = parseTansBitstream_Varint(&bitstream, &tansState, &lo_bytes_read, lo_tans_table);
		instructions++;
	}
	uint16_t* const instructions_end = instructions;
	instructions = instructions_start;

	uint32_t previousNibble = 0;

	while (instructions < instructions_end) {
		const unsigned length = *instructions;
		instructions++;
		const unsigned offset = *instructions;
		instructions++;

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				*dest16 = parseTansBitstream_Deltau16(&bitstream, &tansState, &previousNibble, symbol_tans_table);
				++dest16;
			}
		} else {
			*dest16 = parseTansBitstream_Deltau16(&bitstream, &tansState, &previousNibble, symbol_tans_table);
			++dest16;
			for (unsigned j = 0; j < length; j++) {
				*dest16 = *(dest16 - offset);
				++dest16;
			}
		}
	}

	free(instructions_start);
}

void Smol8UnComp(const struct CompressedData* src, volatile void* dest) {
	//const uint32_t mode = src->data[0] & 0xF;
	//const uint32_t imageSize = (src->data[0] >> 4) | (src->data[1] << 4) | ((src->data[2] & 0x3) << 12);
	const uint32_t symbolsSize = (src->data[2] >> 2) | (src->data[3] << 6);
	const uint32_t lengthoffsetSize = (src->data[4]) | (src->data[5] << 8) | (src->data[6] << 16) | (src->data[7] << 24);

	volatile uint16_t* const dest16Begin = (volatile uint16_t*) dest;
	volatile uint16_t* dest16 = dest16Begin;

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

	volatile uint16_t* const dest16End = dest16;
	dest16 = dest16Begin + 1;
	while (dest16 < dest16End) {
		*dest16 += *(dest16 - 1);
		dest16++;
	}
}
