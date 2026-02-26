#include "memcpy_sram.h"

#include "harness.h"
#include "mgba.h"

void setUp(void){}
void tearDown(void){}

static const char msg1[32] = "Hello World!";
static const char msg2[32] = "All your base are belong to us";

[[gnu::section(".sram")]]
volatile char sram[32] = {0};

[[gnu::section(".sram")]]
volatile char readonly = {0};

void test_is_reading_from_sram(void) {
	TEST_ASSERT_EQUAL_HEX8(0xFF, readonly);
}

void test_msg1(void) {
	memcpy_to_sram(sram, msg1, sizeof(msg1));

	char actual[32] = {0};
	memcpy_from_sram(actual, sram, sizeof(msg1));

	TEST_ASSERT_EQUAL_BYTE_ARRAY(msg1, actual, sizeof(msg1));
}

void test_msg2(void) {
	memcpy_to_sram(sram, msg2, sizeof(msg2));

	char actual[32] = {0};
	memcpy_from_sram(actual, sram, sizeof(msg2));

	TEST_ASSERT_EQUAL_BYTE_ARRAY(msg2, actual, sizeof(msg2));
}

int main() {
	total = 0;
	failed = 0;

	MgbaOpen();

	#ifndef __unix__
		RUN_TEST(test_is_reading_from_sram);
	#endif
	RUN_TEST(test_msg1);
	RUN_TEST(test_msg2);

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return 0 != failed;
}
