#include <stdio.h>
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

[[gnu::noipa]]
void mix_rgb_many_1(
	  volatile rgb_t* dest
	, const rgb_t* from
	, const unsigned count
	, const rgb_t to
	, const unsigned proportion
) {
	// the compiled version does calculate `to.r * (16 - proportion)` only once.
	// No local variables needed.
	for (unsigned i = 0; i < count; ++i) {
		dest[i] = (rgb_t){
			.r = (to.r * (16 - proportion) + from[i].r * proportion) / 16,
			.g = (to.g * (16 - proportion) + from[i].g * proportion) / 16,
			.b = (to.b * (16 - proportion) + from[i].b * proportion) / 16,
		};
	}
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
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	RUN_BENCHMARK(bench_mix_rgb_512_1, check);

	return 0;
}
