#ifndef GBA_BIOS_H
#define GBA_BIOS_H

#include <stdbool.h>
#include <stdint.h>
#include "gba/shared.h"

void IntrWait(bool discard, interrupt_flag_t flags);
void VBlankIntrWait(void);

struct Div {
	int32_t div;
	int32_t rem;
	uint32_t divAbs;
};
struct Div Div(int32_t numer, int32_t denom);

uint16_t Sqrt(uint32_t);

enum CpuSetMode {
	CPU_SET_COPY = 0,
	CPU_SET_FILL = 1,
};
struct CpuSet {
	uint32_t word_count : 21;
	uint32_t : 3;
	enum CpuSetMode mode : 1;
	uint32_t : 1;
	enum WordSize datasize : 1;
};
void CpuSet(const void* src, volatile void* dest, const struct CpuSet);

struct CpuFastSet {
	uint32_t word_count : 21;
	uint32_t unused_0 : 3;
	enum CpuSetMode mode : 1;
};
void CpuFastSet(const void* src, volatile void* dest, const struct CpuFastSet);

[[maybe_unused]]
static void CpuFastCopy(const void* src, volatile void* dest, const uint32_t word_count) {
	CpuFastSet(src, dest, (struct CpuFastSet) {.word_count = word_count, .mode = CPU_SET_COPY});
}
[[maybe_unused]]
static void CpuFastFill(const uint32_t src, volatile void* dest, const uint32_t word_count) {
	CpuFastSet(&src, dest, (struct CpuFastSet) {.word_count = word_count, .mode = CPU_SET_FILL});
}

struct BitUnPack {
	uint16_t src_length;
	uint8_t src_bitsize;
	uint8_t dest_bitsize;
	uint32_t offset : 31;
	bool zero : 1;
};
void BitUnPack(const void* src, volatile void* dest, const struct BitUnPack*);

struct CompressedData;

void LZ77UnCompWram(const struct CompressedData* src, volatile void* dest);
void LZ77UnCompVram(const struct CompressedData* src, volatile void* dest);
void HuffUnComp(const struct CompressedData* src, volatile void* dest);
void RLUnCompWram(const struct CompressedData* src, volatile void* dest);
void RLUnCompVram(const struct CompressedData* src, volatile void* dest);
void Diff8UnFilterWram(const struct CompressedData* src, volatile void* dest);
void Diff8UnFilterVram(const struct CompressedData* src, volatile void* dest);
void Diff16UnFilter(const struct CompressedData* src, volatile void* dest);

#endif
