#include "benchmarks.h"

#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;
char fail_detail[256];

void benchmark_start() {
	reg_timer[2].counter = 0;
	reg_timer[3].counter = 0;
	reg_timer[2].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {
		.timer_enable = true,
		.cascade = true,
		0
	};
	reg_timer[2].control = (timer_control_t) {
		.timer_enable = true,
		0
	};
}

uint32_t benchmark_stop() {
	reg_timer[2].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {0};
	return (reg_timer[3].counter << 16) | (reg_timer[2].counter);
}

void run_benchmark(void (*fn)(void), bool (*check)(void), const char* name) {
	setUp();
	VBlankIntrWait();
	benchmark_start();
	fn();
	uint32_t time = benchmark_stop();
	bool currentTestSucceeded = check();
	tearDown();
	if (currentTestSucceeded) {
		MgbaPrintf(MGBA_LOG_INFO, "%s: \033[44mBENCH\033[0m: %ld cycles = %ld.%03ld frames",
			name, time, time / CYCLES_PER_FRAME, (time * 1000 / CYCLES_PER_FRAME) % 1000);
	} else {
		++failed;
		MgbaPrintf(MGBA_LOG_INFO, "%s: \033[41mFAIL\033[0m: %s",
			name, fail_detail);
	}
	++total;
}
