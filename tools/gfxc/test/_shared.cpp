#include "_shared.hpp"

#include <cstdio>

bool currentTestFailed;
unsigned total;
unsigned failed;

static void print_vector_hex8(std::vector<uint8_t> value) {
	printf("{");
	auto i = value.begin();
	if (i != value.end()) {
		printf("%02X", *i);
		i++;
		for (; i != value.end(); i++) {
			printf(", %02X", *i);
		}
	}
	printf("}");
}

void TEST_ASSERT_EQUAL_VECTOR_HEX8(std::vector<uint8_t> expected, std::vector<uint8_t> actual) {
	if (expected != actual) {
		currentTestFailed = 1;
		printf("\033[41mFAIL\033[0m: Expected \n  ");
		print_vector_hex8(expected);
		printf("; was \n  ");
		print_vector_hex8(actual);
	}
}

void TEST_ASSERT_EQUAL_UNSIGNED(unsigned expected, unsigned actual) {
	if (expected != actual) {
		currentTestFailed = 1;
		printf("\033[41mFAIL\033[0m: Expected %d; was %d", expected, actual);
	}
}

void TEST_FAIL(std::string reason) {
	currentTestFailed = 1;
	std::string msg("\033[41mFAIL\033[0m: ");
	msg += reason;
	printf("%s", msg.c_str());
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
