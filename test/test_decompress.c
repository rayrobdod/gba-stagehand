#include "decompress/by_header.h"

#include <stddef.h>
#include <stdio.h>
#include "gba/bios.h"
#include "gba/vram.h"
#include "management/isr.h"
#include "harness.h"
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

static const uint32_t initial_memory_fill = 0x12345678;

void setUp(void){
	CpuFastSet(&initial_memory_fill, vram.screenblock[0], (struct CpuFastSet) {.word_count = sizeof(vram) / sizeof(uint32_t), .mode = CPU_SET_FILL});
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


const char* compressed_0;
const char* raw_0;
unsigned length_0;

void run_decompress_test_suite0(void) {
	HeaderUnCompVram(compressed_0, vram.screenblock[0]);
	TEST_ASSERT_EQUAL_BYTE_ARRAY(raw_0, (char*) vram.screenblock[0], length_0);
	TEST_ASSERT_EQUAL_HEX8(initial_memory_fill >> (8 * (length_0 % 4)), ((char*) vram.screenblock[0])[length_0]);
}

void run_decompress_test_suite(const struct compression_suite * suite, const char* name) {
	if (0x00 == suite->data[0]) {
		raw_0 = (suite->data + 4);
		length_0 = *((uint32_t*) suite->data) >> 8;

		char name_0[120];

		while (suite->data != NULL) {
			compressed_0 = suite->data;

			snprintf(name_0, 119, "%s %6s", name, UnCompFnName(suite->data[0]));

			run_test(run_decompress_test_suite0, name_0);
			++suite;
		}
	} else {
		MgbaPrintf(MGBA_LOG_INFO, "%s: \033[43mWARN\033[0m: no identity to compare to", name);
	}
}
#define RUN_DECOMPRESS_TEST_SUITE(func) run_decompress_test_suite(func, #func);


int main(int argc, char** argv) {
	total = 0;
	failed = 0;

	MgbaOpen();

	RUN_DECOMPRESS_TEST_SUITE(compression_suite_snow_mountain_under_stars);
	RUN_DECOMPRESS_TEST_SUITE(compression_suite_arrow_left);
	RUN_DECOMPRESS_TEST_SUITE(compression_suite_ball);
	RUN_DECOMPRESS_TEST_SUITE(compression_suite_brickbreak_background_tiles);
	RUN_DECOMPRESS_TEST_SUITE(compression_suite_brickbreak_background_map);

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return 0 != failed;
}
