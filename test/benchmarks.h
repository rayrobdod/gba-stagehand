#include <stdbool.h>
#include <stdint.h>
#include "gba/screen.h"

static const unsigned CYCLES_PER_FRAME = 280896;
static const unsigned CYCLES_PER_VBLANK = CYCLES_PER_FRAME * (VCOUNT_MAX - DISPLAY_HEIGHT) / VCOUNT_MAX;

extern char fail_detail[256];

void setUp(void);
void tearDown(void);

void benchmark_start(void);
uint32_t benchmark_stop(void);
void run_benchmark(void (*fn)(void), bool (*check)(void), const char* name);

#define RUN_BENCHMARK(func, check) run_benchmark(func, check, #func);
