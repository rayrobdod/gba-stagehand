#ifndef DECOMPRESS_TYPE_H
#define DECOMPRESS_TYPE_H

#include <stdint.h>

struct CompressedData {
	uint32_t magic : 8;
	uint32_t size : 24;
	uint8_t data[];
};

struct suspended_decompression {
	const uint8_t* src;
	const uint8_t* src_ptrs[2];
	volatile uint8_t* dest;
	volatile uint8_t* dest_end;
	uint8_t magic;
	uint16_t regs[4];
};

#endif        //  #ifndef DECOMPRESS_TYPE_H
