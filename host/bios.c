#include "gba/bios.h"
#include "mgba.h"
#include <math.h>
#include <stdlib.h>

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

void LZ77UnCompVram(const void* src, volatile void* dest) {
	// ???
}

void BitUnPack(const void* src, volatile void* dest, const struct BitUnPack*) {
	// ???
}


void HeaderUnCompVram(const void* src, volatile void* dest) {
	// ???
}


#include <stdarg.h>
#include <stdio.h>

void MgbaPrintf(enum MgbaLogLevel level, const char* ptr, ...)
{
	va_list args;

	level &= 0x7;
	va_start(args, ptr);
	vprintf(ptr, args);
	va_end(args);
}

