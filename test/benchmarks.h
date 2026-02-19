#include <stdbool.h>
#include <stdint.h>
#include "gba/screen.h"
#include "management/transition.h"

static const unsigned CYCLES_PER_FRAME = 280896;
static const unsigned CYCLES_PER_VBLANK = CYCLES_PER_FRAME * (VCOUNT_MAX - DISPLAY_HEIGHT) / VCOUNT_MAX;

extern char fail_detail[256];

void setUp(void);
void tearDown(void);

void benchmark_start(void);
uint32_t benchmark_stop(void);
void run_benchmark(void (*fn)(void), bool (*check)(void), const char* name);

typedef struct {
	bool all_frames : 1;
	bool vram_ops : 1;
} run_transition_benchmark_verbosity_t;

[[maybe_unused]] static const run_transition_benchmark_verbosity_t RTBV_SUMMARY_ONLY = {0};
[[maybe_unused]] static const run_transition_benchmark_verbosity_t RTBV_ALL_FRAMES = {.all_frames = true};

void run_transition_benchmark(
	const struct transition*,
	const struct transitionSourceCallbacks*,
	const struct transitionTargetCallbacks*,
	const char* name,
	run_transition_benchmark_verbosity_t);

#define RUN_BENCHMARK(func, check) run_benchmark(func, check, #func);
