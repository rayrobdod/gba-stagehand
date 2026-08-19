#include <stdio.h>
#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/vram.h"
#include "management/isr.h"
#include "utils/arraycount.h"
#include "benchmarks.h"
#include "graphics.h"
#include "mgba.h"

union rgb2uint {
	rgb_t rgb;
	uint16_t uint;
};

void setUp(void){
	CpuFastFill(0,
		vram.mode3,
		240 * 160 / 2);
}
void tearDown(void){}

bool check(void) {
	for (int y = 0; y < 160; y++)
	for (int x = 0; x < 240; x++) {
		union rgb2uint expected;
		expected.rgb = snow_mountain_under_stars[y][x];
		union rgb2uint actual;
		actual.rgb = vram.mode3[y][x];

		if (actual.uint != expected.uint) {
			snprintf(fail_detail, arraycount(fail_detail), "difference at [%d][%d]: was %04x, expected %04x", y, x, actual.uint, expected.uint);
			return false;
		}
	}
	return true;
}


static const struct reg_dma dma_mountain = {
	.source = snow_mountain_under_stars,
	.destination = vram.mode3,
	.word_count = 240 * 160,
	.control = {
		.dest_control = DMA_ADDR_INCREMENT,
		.src_control = DMA_ADDR_INCREMENT,
		.repeat = false,
		.word_size = WORDSIZE_16BIT,
		.start = DMA_START_IMMEDIATELY,
		.irq = false,
		.enable = true,
	},
};
void bench_dma(void) {
	reg_dma[3] = dma_mountain;
}

void bench_cpuset(void) {
	CpuFastSet(
		snow_mountain_under_stars,
		vram.mode3,
		(struct CpuFastSet){
			.word_count = 240 * 160 / 2,
			.mode = CPU_SET_COPY,
		});
}

int main() {
	reg_interrupt.WAITCNT = common_waitcnt;
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	RUN_BENCHMARK(bench_dma, check);
	RUN_BENCHMARK(bench_cpuset, check);

	return 0;
}
