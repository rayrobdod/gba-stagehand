#include "decompress/by_header.h"

#include <stddef.h>
#include "management/isr.h"
#include "gba/bios.h"
#include "gba/vram.h"
#include "benchmarks.h"
#include "mgba.h"

struct decompression_suite {
	const char* label;
	const char* raw;
	const char* data;
	uint32_t size;
};

const extern struct decompression_suite __decompression_suite_array_start[];
const extern struct decompression_suite __decompression_suite_array_end[];

static const uint32_t zero_uint32 = 0;

__attribute__((section(".bss")))
static char wram_buffer[1024 * 24];

void setUp(void){
	CpuFastSet(&zero_uint32, vram.screenblock[0], (struct CpuFastSet) {.word_count = sizeof(vram) / sizeof(uint32_t), .mode = CPU_SET_FILL});
	CpuFastSet(&zero_uint32, wram_buffer, (struct CpuFastSet) {.word_count = sizeof(wram_buffer) / sizeof(uint32_t), .mode = CPU_SET_FILL});
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

void run_decompress_benchmark(const struct decompression_suite * suite) {
	uint32_t raw_length = *((uint32_t*) suite->data) >> 8;

	if (0 == raw_length % 2) {
		setUp();
		VBlankIntrWait();
		benchmark_start();
		HeaderUnCompVram(suite->data, vram.screenblock[0]);
		uint32_t time = benchmark_stop();
		tearDown();
		MgbaPrintf(MGBA_LOG_INFO, "Decompress Vram: %s %6s: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames (%6ld bytes)",
			suite->label, UnCompFnName(suite->data[0]), time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000, suite->size);
	}

	if (raw_length <= sizeof(wram_buffer)) {
		setUp();
		VBlankIntrWait();
		benchmark_start();
		HeaderUnCompWram(suite->data, wram_buffer);
		uint32_t time = benchmark_stop();
		tearDown();
		MgbaPrintf(MGBA_LOG_INFO, "Decompress Wram: %s %6s: \033[44mBENCH\033[0m: %8ld cycles = %2ld.%03ld frames (%6ld bytes)",
			suite->label, UnCompFnName(suite->data[0]), time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000, suite->size);
	}
}


int main(int argc, char** argv) {
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	const struct decompression_suite* it = __decompression_suite_array_start;
	while (it < __decompression_suite_array_end) {
		run_decompress_benchmark(it);
		it++;
	}

	return 0;
}
