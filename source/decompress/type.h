#ifndef DECOMPRESS_TYPE_H
#define DECOMPRESS_TYPE_H

#include <stdint.h>

struct CompressedData {
	uint32_t magic : 8;
	uint32_t size : 24;
	uint8_t data[];
};

#endif        //  #ifndef DECOMPRESS_TYPE_H
