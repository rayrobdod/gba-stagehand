#include "gba/bios.h"
#include "mgba.h"
#include <math.h>
#include <stdlib.h>
#include "decompress/type.h"

void VBlankIntrWait() {
}

struct Div Div(int32_t numer, int32_t denom) {
	return (struct Div) {
		numer / denom,
		numer % denom,
		abs(numer / denom),
	};
}

uint16_t Sqrt(uint32_t num) {
	return (uint16_t) sqrt((double) num);
}

void CpuSet(const void* src, volatile void* dest, const struct CpuSet args) {
	if (args.mode == CPU_SET_COPY) {
		if (args.datasize == WORDSIZE_16BIT) {
			uint16_t* src16 = (uint16_t*) src;
			uint16_t* dest16 = (uint16_t*) dest;
			for (int i = 0; i < args.word_count; i++) {
				dest16[i] = src16[i];
			}
		} else {
			uint32_t* src32 = (uint32_t*) src;
			uint32_t* dest32 = (uint32_t*) dest;
			for (int i = 0; i < args.word_count; i++) {
				dest32[i] = src32[i];
			}
		}
	} else {
		if (args.datasize == WORDSIZE_16BIT) {
			uint16_t v = *((uint16_t*) src);
			uint16_t* dest16 = (uint16_t*) dest;
			for (int i = 0; i < args.word_count; i++) {
				dest16[i] = v;
			}
		} else {
			uint32_t v = *((uint32_t*) src);
			uint32_t* dest32 = (uint32_t*) dest;
			for (int i = 0; i < args.word_count; i++) {
				dest32[i] = v;
			}
		}
	}
}

void CpuFastSet(const void* src, volatile void* dest, const struct CpuFastSet args) {
	if (args.mode == CPU_SET_COPY) {
		uint32_t* src32 = (uint32_t*) src;
		uint32_t* dest32 = (uint32_t*) dest;
		for (int i = 0; i < args.word_count;) {
			dest32[i] = src32[i];
			i++;
			dest32[i] = src32[i];
			i++;
			dest32[i] = src32[i];
			i++;
			dest32[i] = src32[i];
			i++;
			dest32[i] = src32[i];
			i++;
			dest32[i] = src32[i];
			i++;
			dest32[i] = src32[i];
			i++;
			dest32[i] = src32[i];
			i++;
		}
	} else {
		uint32_t src32 = *((uint32_t*) src);
		uint32_t* dest32 = (uint32_t*) dest;
		for (int i = 0; i < args.word_count;) {
			dest32[i] = src32;
			i++;
			dest32[i] = src32;
			i++;
			dest32[i] = src32;
			i++;
			dest32[i] = src32;
			i++;
			dest32[i] = src32;
			i++;
			dest32[i] = src32;
			i++;
			dest32[i] = src32;
			i++;
			dest32[i] = src32;
			i++;
		}
	}
}

void BitUnPack(__attribute__((unused)) const void* src, __attribute__((unused)) volatile void* dest, __attribute__((unused)) const struct BitUnPack*) {
	// ???
}

void LZ77UnCompWram(const struct CompressedData* src, volatile void* dest) {
	volatile uint8_t* dest8 = (volatile uint8_t*)dest;
	volatile uint8_t* const dest_end = dest + (src->size / sizeof(uint8_t));

	const uint8_t* src8 = src->data;

	while (dest8 < dest_end) {
		unsigned flags = *(src8++);

		for (int i = 7; i >= 0; --i) {
			if (dest8 >= dest_end)
				break;

			if (flags & (1 << i)) {
				unsigned width = (*src8 >> 4) + 3;
				unsigned distance = (((*src8 & 0xF) << 8) | (*(src8 + 1))) + 1;
				src8 += 2;

				volatile uint8_t* from = dest8 - distance;
				for (; width > 0; --width) {
					*dest8++ = *from++;
				}
			} else {
				*dest8++ = *src8++;
			}
		}
	}
}
void LZ77UnCompVram(const struct CompressedData* src, volatile void* dest) {
	LZ77UnCompWram(src, dest);
}
void HuffUnComp([[maybe_unused]] const struct CompressedData* src, [[maybe_unused]] volatile void* dest) {}
void RLUnCompWram([[maybe_unused]] const struct CompressedData* src, [[maybe_unused]] volatile void* dest) {}
void RLUnCompVram(const struct CompressedData* src, volatile void* dest) {
	RLUnCompWram(src, dest);
}
void Diff8UnFilterWram([[maybe_unused]] const struct CompressedData* src, [[maybe_unused]] volatile void* dest) {}
void Diff8UnFilterVram(const struct CompressedData* src, volatile void* dest) {
	Diff8UnFilterWram(src, dest);
}
void Diff16UnFilter([[maybe_unused]] const struct CompressedData* src, [[maybe_unused]] volatile void* dest) {}



#include <stdarg.h>
#include <stdio.h>

bool32 MgbaOpen(void) {
	return true;
}

void MgbaPrintf(enum MgbaLogLevel level, const char* ptr, ...) {
	va_list args;

	level &= 0x7;
	va_start(args, ptr);
	vprintf(ptr, args);
	va_end(args);
	printf("\n");
}

