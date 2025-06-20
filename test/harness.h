#include "gba/palette.h"

void TEST_ASSERT_EQUAL_UNSIGNED(unsigned expected, unsigned actual);
void TEST_ASSERT_EQUAL_RGB(rgb_t expected, rgb_t actual);
void TEST_FAIL(const char* reason);

extern unsigned total;
extern unsigned failed;

void run_test(void (*fn)(void), const char* name);

#define RUN_TEST(func) run_test(func, #func);
