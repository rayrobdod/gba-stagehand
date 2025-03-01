#include <cstdio>
#include "subword_output_iterator.hpp"

bool currentTestFailed;
unsigned total;
unsigned failed;

static void print_vector_hex8(std::vector<uint8_t> value) {
	printf("{");
	auto i = value.begin();
	if (i != value.end()) {
		printf("%X", *i);
		i++;
		for (; i != value.end(); i++) {
			printf(", %X", *i);
		}
	}
	printf("}");
}

static void TEST_ASSERT_EQUAL_VECTOR_HEX8(std::vector<uint8_t> expected, std::vector<uint8_t> actual) {
	if (expected != actual) {
		currentTestFailed = 1;
		printf("\033[41mFAIL\033[0m: Expected ");
		print_vector_hex8(expected);
		printf("; was ");
		print_vector_hex8(actual);
	}
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
#define RUN_TEST(func) run_test(func, #func);

void test_4bpp_10(void) {
	subword_output_iterator dut(4);
	for (unsigned i = 0; i < 10; i++) {
		*(dut++) = i;
	}
	std::vector<uint8_t> expected({0x10, 0x32, 0x54, 0x76, 0x98});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}
void test_4bpp_16(void) {
	subword_output_iterator dut(4);
	for (unsigned i = 0; i < 16; i++) {
		*(dut++) = i;
	}
	std::vector<uint8_t> expected({0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}
void test_2bpp_16(void) {
	subword_output_iterator dut(2);
	for (unsigned i = 0; i < 16; i++) {
		*(dut++) = i & 0x3;
	}
	std::vector<uint8_t> expected({0xE4, 0xE4, 0xE4, 0xE4});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}
void test_1bpp(void) {
	subword_output_iterator dut(1);
	*(dut++) = 0;
	*(dut++) = 1;
	*(dut++) = 0;
	*(dut++) = 0;
	*(dut++) = 1;
	*(dut++) = 1;
	*(dut++) = 0;
	*(dut++) = 0;
	*(dut++) = 0;
	*(dut++) = 1;
	*(dut++) = 1;
	*(dut++) = 1;
	std::vector<uint8_t> expected({0b00110010, 0b1110});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}

int main(int argc, char** argv) {
	total = 0;
	failed = 0;

	RUN_TEST(test_4bpp_10);
	RUN_TEST(test_4bpp_16);
	RUN_TEST(test_2bpp_16);
	RUN_TEST(test_1bpp);

	printf("Total: %d; Failing: %d\n", total, failed);
	return 0 != failed;
}

#include "subword_output_iterator.cpp"
