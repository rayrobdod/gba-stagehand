#include "gba/palette.h"

void TEST_ASSERT_EQUAL_UNSIGNED(unsigned expected, unsigned actual);
void TEST_ASSERT_EQUAL_HEX8(uint8_t expected, uint8_t actual);
void TEST_ASSERT_EQUAL_BYTE_ARRAY(const char* expected, const char* actual, unsigned length);
void TEST_ASSERT_EQUAL_RGB(rgb_t expected, rgb_t actual);
void TEST_ASSERT(bool actual, const char* reason);
void TEST_FAIL(const char* reason);

void setUp(void);
void tearDown(void);

extern unsigned total;
extern unsigned failed;

void run_test(void (*fn)(void), const char* name);

#define RUN_TEST(func) run_test(func, #func);
