#include "decompress/by_header.h"

#include <stddef.h>
#include "management/isr.h"
#include "gba/bios.h"
#include "gba/vram.h"
#include "benchmarks.h"
#include "mgba.h"

struct compression_suite {
	const char* data;
	uint32_t size;
};

const extern struct compression_suite compression_suite_snow_mountain_under_stars[];
const extern struct compression_suite compression_suite_arrow_left[];
const extern struct compression_suite compression_suite_ball[];
const extern struct compression_suite compression_suite_brickbreak_background_tiles[];
const extern struct compression_suite compression_suite_brickbreak_background_map[];

static const uint32_t zero_uint32 = 0;

void setUp(void){
	CpuFastSet(&zero_uint32, &vram, (struct CpuFastSet) {.word_count = sizeof(vram) / sizeof(uint32_t), .mode = CPU_SET_FILL});
}
void tearDown(void){}

static const char* UnCompFnName(unsigned magic) {
	switch (magic) {
	case 0x00:
		return "IDENT";
	case 0x10:
		return "LZ";
	case 0x11:
		return "LZ11";
	case 0x24:
		return "Huff4";
	case 0x28:
		return "Huff8";
	case 0x30:
		return "RL";
	case 0x41:
		return "Frit8";
	case 0x42:
		return "Frit16";
	case 0x81:
		return "Diff8";
	case 0x82:
		return "Diff16";
	default:
		return "???";
	}
}

void run_decompress_benchmark_suite(const struct compression_suite * suite, const char* name) {
	while (suite->data != NULL) {
		setUp();
		VBlankIntrWait();
		benchmark_start();
		HeaderUnCompVram(suite->data, vram.screenblock[0]);
		uint32_t time = benchmark_stop();
		tearDown();
		MgbaPrintf(MGBA_LOG_INFO, "%s %6s: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames (%6ld bytes)",
			name, UnCompFnName(suite->data[0]), time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000, suite->size);

		++suite;
	}
}
#define RUN_DECOMPRESS_BENCHMARK_SUITE(func) run_decompress_benchmark_suite(func, #func);


int main(int argc, char** argv) {
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	RUN_DECOMPRESS_BENCHMARK_SUITE(compression_suite_snow_mountain_under_stars);
	RUN_DECOMPRESS_BENCHMARK_SUITE(compression_suite_arrow_left);
	RUN_DECOMPRESS_BENCHMARK_SUITE(compression_suite_ball);
	RUN_DECOMPRESS_BENCHMARK_SUITE(compression_suite_brickbreak_background_tiles);
	RUN_DECOMPRESS_BENCHMARK_SUITE(compression_suite_brickbreak_background_map);

	return 0;
}
