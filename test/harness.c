#include "harness.h"

#include <stdio.h>

bool currentTestFailed;
unsigned total;
unsigned failed;

void TEST_ASSERT_EQUAL_UNSIGNED(unsigned expected, unsigned actual) {
	if (expected != actual) {
		currentTestFailed = 1;
		printf("\033[41mFAIL\033[0m: Expected %d; was %d", expected, actual);
	}
}

static void print_rgb(rgb_t value) {
	printf("{.r = %d, .g = %d, .b = %d}", value.r, value.g, value.b);
}

void TEST_ASSERT_EQUAL_RGB(rgb_t expected, rgb_t actual) {
	if (expected.r != actual.r || expected.g != actual.g || expected.b != actual.b) {
		currentTestFailed = 1;
		printf("\033[41mFAIL\033[0m: Expected ");
		print_rgb(expected);
		printf("; was ");
		print_rgb(actual);
	}
}

void TEST_FAIL(const char* reason) {
	currentTestFailed = 1;
	printf("\033[41mFAIL\033[0m: %s", reason);
}

void run_test(void (*fn)(void), const char* name) {
	printf("%s: ", name);
	currentTestFailed = 0;
	fn();
	if (currentTestFailed) {
		++failed;
	} else {
		printf("\033[42mPASS\033[0m");
	}
	++total;
	printf("\n");
}
