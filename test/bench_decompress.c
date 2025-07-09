#include "decompress/by_header.h"

#include <stddef.h>
#include <stdio.h>
#include "decompress/type.h"
#include "management/isr.h"
#include "gba/bios.h"
#include "gba/vram.h"
#include "utils/arraycount.h"
#include "benchmarks.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;
static char fail_detail[256];

struct decompression_suite {
	const char* label;
	const char* raw;
	const struct CompressedData* data;
	uint32_t size;
};

const extern struct decompression_suite __decompression_suite_array_start[];
const extern struct decompression_suite __decompression_suite_array_end[];

static const uint32_t initial_memory_fill = 0x12345678;

__attribute__((section(".bss")))
static char iwram_buffer[1024 * 24];

__attribute__((section(".sbss")))
static char ewram_buffer[1024 * 96];

__attribute__((noinline))
void setUp(void){
	CpuFastSet(&initial_memory_fill, vram.screenblock[0], (struct CpuFastSet) {.word_count = sizeof(vram) / sizeof(uint32_t), .mode = CPU_SET_FILL});
	CpuFastSet(&initial_memory_fill, iwram_buffer, (struct CpuFastSet) {.word_count = sizeof(iwram_buffer) / sizeof(uint32_t), .mode = CPU_SET_FILL});
	CpuFastSet(&initial_memory_fill, ewram_buffer, (struct CpuFastSet) {.word_count = sizeof(ewram_buffer) / sizeof(uint32_t), .mode = CPU_SET_FILL});
}
void tearDown(void){}


bool assert_equal_array_bound(const char* expected, const char* actual, unsigned length) {
	unsigned i;
	for (i = 0; i < length; i++) {
		if (expected[i] != actual[i]) {
			snprintf(fail_detail, arraycount(fail_detail), "At %d: Expected %d; was %d", i, expected[i], actual[i]);
			return true;
		}
	}
	char expected_b = initial_memory_fill >> (8 * (length % 4));
	if (expected_b != actual[i]) {
		snprintf(fail_detail, arraycount(fail_detail), "overwrote expected bounds at %d: was %d", i, actual[i]);
		return true;
	}
	return false;
}

__attribute__((noinline))
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

void run_decompress_benchmark(const struct decompression_suite * suite) {
	uint32_t raw_length = *((uint32_t*) suite->data) >> 8;

	if (0 == raw_length % 2) {
		setUp();
		VBlankIntrWait();
		benchmark_start();
		HeaderUnCompVram(suite->data, vram.screenblock[0]);
		uint32_t time = benchmark_stop();
		bool currentTestFailed = assert_equal_array_bound(suite->raw, (char*) vram.screenblock[0], raw_length);
		tearDown();
		if (currentTestFailed) {
			++failed;
			MgbaPrintf(MGBA_LOG_INFO, "Decompress  Vram: %s %6s: \033[41mFAIL\033[0m: %s",
				suite->label, UnCompFnName(suite->data->magic), fail_detail);
		} else {
			MgbaPrintf(MGBA_LOG_INFO, "Decompress  Vram: %s %6s: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames (%6ld bytes)",
				suite->label, UnCompFnName(suite->data->magic), time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000, suite->size);
		}
		++total;
	}

	if (raw_length <= sizeof(iwram_buffer)) {
		setUp();
		VBlankIntrWait();
		benchmark_start();
		HeaderUnCompWram(suite->data, iwram_buffer);
		uint32_t time = benchmark_stop();
		bool currentTestFailed = assert_equal_array_bound(suite->raw, iwram_buffer, raw_length);
		tearDown();
		if (currentTestFailed) {
			++failed;
			MgbaPrintf(MGBA_LOG_INFO, "Decompress IWram: %s %6s: \033[41mFAIL\033[0m: %s",
				suite->label, UnCompFnName(suite->data->magic), fail_detail);
		} else {
			MgbaPrintf(MGBA_LOG_INFO, "Decompress IWram: %s %6s: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames (%6ld bytes)",
				suite->label, UnCompFnName(suite->data->magic), time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000, suite->size);
		}
		++total;
	} else {
		setUp();
		VBlankIntrWait();
		benchmark_start();
		HeaderUnCompWram(suite->data, ewram_buffer);
		uint32_t time = benchmark_stop();
		bool currentTestFailed = assert_equal_array_bound(suite->raw, ewram_buffer, raw_length);
		tearDown();
		if (currentTestFailed) {
			++failed;
			MgbaPrintf(MGBA_LOG_INFO, "Decompress EWram: %s %6s: \033[41mFAIL\033[0m: %s",
				suite->label, UnCompFnName(suite->data->magic), fail_detail);
		} else {
			MgbaPrintf(MGBA_LOG_INFO, "Decompress EWram: %s %6s: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames (%6ld bytes)",
				suite->label, UnCompFnName(suite->data->magic), time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000, suite->size);
		}
		++total;
	}
}


int main(int argc, char** argv) {
	total = 0;
	failed = 0;

	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	const struct decompression_suite* it = __decompression_suite_array_start;
	while (it < __decompression_suite_array_end) {
		run_decompress_benchmark(it);
		it++;
	}

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return failed != 0;
}
