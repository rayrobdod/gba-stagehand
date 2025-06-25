#include "decompress/by_header.h"

#include <stddef.h>
#include <stdio.h>
#include "gba/bios.h"
#include "gba/vram.h"
#include "management/isr.h"
#include "harness.h"
#include "mgba.h"

struct dedecompression_suite {
	const char* label;
	const char* raw;
	const char* data;
	uint32_t size;
};

const extern struct dedecompression_suite __decompression_suite_array_start[];
const extern struct dedecompression_suite __decompression_suite_array_end[];

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

void run_decompress_test(const struct dedecompression_suite * suite) {
	raw_0 = suite->raw;
	length_0 = *((uint32_t*) suite->data) >> 8;
	compressed_0 = suite->data;

	char name_0[120];
	snprintf(name_0, 119, "Decompress %s %6s", suite->label, UnCompFnName(suite->data[0]));

	run_test(run_decompress_test_suite0, name_0);
}

int main(int argc, char** argv) {
	total = 0;
	failed = 0;

	MgbaOpen();

	const struct dedecompression_suite* it = __decompression_suite_array_start;
	while (it < __decompression_suite_array_end) {
		run_decompress_test(it);
		it++;
	}

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return 0 != failed;
}
