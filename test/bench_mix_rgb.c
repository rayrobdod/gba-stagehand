#include "mix_rgb.h"

#include <stdio.h>
#include "gba/hw_reg.h"
#include "gba/palette.h"
#include "management/isr.h"
#include "utils/arraycount.h"
#include "benchmarks.h"
#include "mgba.h"

union rgb2uint {
	rgb_t rgb;
	uint16_t uint;
};

[[gnu::section(".sbss")]]
static rgb_t random_pal_512[512] = {0};

void setUp(void) {
	static uint32_t lfsr = 159226;

	for (unsigned i = 0; i < arraycount(random_pal_512); i++) {
		union rgb2uint v = {.uint = lfsr};
		random_pal_512[i] = v.rgb;

		lfsr = (lfsr >> 1) ^ (lfsr & 1 ? 0xB40000 : 0);
	}
}
void tearDown(void) {}
bool check(void) {
	return true;
}

void bench_mix_rgb_512_1(void) {
	mix_rgb_many_1(
		hw_palette.all,
		random_pal_512,
		512,
		rgb(11, 3, 20),
		7
	);
}

int main() {
	reg_interrupt.WAITCNT = common_waitcnt;
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	RUN_BENCHMARK(bench_mix_rgb_512_1, check);

	return 0;
}
