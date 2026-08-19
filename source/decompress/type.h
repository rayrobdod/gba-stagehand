#ifndef DECOMPRESS_TYPE_H
#define DECOMPRESS_TYPE_H

#include <stdint.h>

struct CompressedData {
	uint32_t magic : 8;
	uint32_t size : 24;
	uint8_t data[];
};

struct smol_bitstream {
	const uint16_t* src;
	unsigned buffer;
	unsigned buffer_size;
};

struct suspended_decompression {
	const uint8_t* src;
	volatile uint8_t* dest;
	volatile uint8_t* dest_end;
	union {
		struct {
			uint8_t flags;
			uint8_t flag_counter;
		} lz11;
		struct {
			uint16_t flags;
			uint8_t flag_counter;
		} lz16;
		struct {
			const uint8_t* src_tree;
			uint16_t inIntraOffset;
		} huff;
		struct {
			uint16_t regs[4];
		} frit;
		struct {
			const uint8_t* src_end;
			uint32_t lengthoffsetSize;
			const uint16_t* symbols;
			const uint32_t* tans_table;
			struct smol_bitstream bitstream;
			uint8_t tansState;
			uint8_t previousNibble;
		} smol;
	};
	uint8_t magic;
};

#endif        //  #ifndef DECOMPRESS_TYPE_H
