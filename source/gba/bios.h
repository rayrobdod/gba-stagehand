#include <stdbool.h>
#include <stdint.h>

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
enum CpuSetSize {
	CPU_SET_16BIT = 0,
	CPU_SET_32BIT = 1,
};
struct CpuSet {
	uint32_t word_count : 21;
	uint32_t unused_0 : 3;
	enum CpuSetMode mode : 1;
	uint32_t unused_1 : 1;
	enum CpuSetSize datasize : 1;
};
void CpuSet(const void* src, volatile void* dest, const struct CpuSet);

struct CpuFastSet {
	uint32_t word_count : 21;
	uint32_t unused_0 : 3;
	enum CpuSetMode mode : 1;
};
void CpuFastSet(const void* src, volatile void* dest, const struct CpuFastSet);

struct BitUnPack {
	uint16_t src_length;
	uint8_t src_bitsize;
	uint8_t dest_bitsize;
	uint32_t offset : 31;
	bool zero : 1;
};
void BitUnPack(const void* src, volatile void* dest, const struct BitUnPack*);
