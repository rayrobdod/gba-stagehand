#include <stdint.h>

static const unsigned CYCLES_PER_FRAME = 280896;

void setUp(void);
void tearDown(void);

void benchmark_start();
uint32_t benchmark_stop();
void run_benchmark(void (*fn)(void), const char* name);

#define RUN_BENCHMARK(func) run_benchmark(func, #func);
