#include "harness.h"

#include <stdio.h>
#include "mgba.h"
#include "utils/arraycount.h"

bool currentTestFailed;
unsigned total;
unsigned failed;

static char fail_detail[256];

void TEST_ASSERT_EQUAL_UNSIGNED(unsigned expected, unsigned actual) {
	if (expected != actual) {
		currentTestFailed = 1;
		snprintf(fail_detail, arraycount(fail_detail), "Expected %d; was %d", expected, actual);
	}
}

void TEST_ASSERT_EQUAL_BYTE_ARRAY(const char* expected, const char* actual, unsigned length) {
	for (unsigned i = 0; i < length; i++) {
		if (expected[i] != actual[i]) {
			currentTestFailed = 1;
			snprintf(fail_detail, arraycount(fail_detail), "At %d: Expected %d; was %d", i, expected[i], actual[i]);
			break;
		}
	}
}

static int snprint_rgb(char* buf, size_t size, rgb_t value) {
	return snprintf(buf, size, "{.r = %d, .g = %d, .b = %d}", value.r, value.g, value.b);
}

void TEST_ASSERT_EQUAL_RGB(rgb_t expected, rgb_t actual) {
	if (expected.r != actual.r || expected.g != actual.g || expected.b != actual.b) {
		currentTestFailed = 1;
		size_t offset = snprintf(fail_detail, arraycount(fail_detail), "Expected ");
		offset += snprint_rgb(fail_detail + offset, arraycount(fail_detail) - offset, expected);
		offset += snprintf(fail_detail + offset, arraycount(fail_detail), "; was ");
		offset += snprint_rgb(fail_detail + offset, arraycount(fail_detail) - offset, actual);
	}
}

void TEST_FAIL(const char* reason) {
	currentTestFailed = 1;
	snprintf(fail_detail, arraycount(fail_detail), "%s", reason);
}

void run_test(void (*fn)(void), const char* name) {
	currentTestFailed = 0;
	fn();
	if (currentTestFailed) {
		++failed;
		MgbaPrintf(MGBA_LOG_INFO, "%s: \033[41mFAIL\033[0m: %s", name, fail_detail);
	} else {
		MgbaPrintf(MGBA_LOG_INFO, "%s: \033[42mPASS\033[0m", name);
	}
	++total;
}
