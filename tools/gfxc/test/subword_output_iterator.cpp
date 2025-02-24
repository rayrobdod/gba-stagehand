#include "subword_output_iterator.hpp"

#include <cstdio>

bool currentTestFailed;
unsigned total;
unsigned failed;

static void print_vector_hex32(std::vector<uint32_t> value) {
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

static void TEST_ASSERT_EQUAL_VECTOR_HEX32(std::vector<uint32_t> expected, std::vector<uint32_t> actual) {
	if (expected != actual) {
		currentTestFailed = 1;
		printf("\033[41mFAIL\033[0m: Expected ");
		print_vector_hex32(expected);
		printf("; was ");
		print_vector_hex32(actual);
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
	std::vector<uint32_t> expected({0x76543210, 0x00000098});
	TEST_ASSERT_EQUAL_VECTOR_HEX32(expected, dut.result());
}
void test_4bpp_16(void) {
	subword_output_iterator dut(4);
	for (unsigned i = 0; i < 16; i++) {
		*(dut++) = i;
	}
	std::vector<uint32_t> expected({0x76543210, 0xFEDCBA98});
	TEST_ASSERT_EQUAL_VECTOR_HEX32(expected, dut.result());
}
void test_2bpp_16(void) {
	subword_output_iterator dut(2);
	for (unsigned i = 0; i < 16; i++) {
		*(dut++) = i & 0x3;
	}
	std::vector<uint32_t> expected({0xE4E4E4E4});
	TEST_ASSERT_EQUAL_VECTOR_HEX32(expected, dut.result());
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
	std::vector<uint32_t> expected({0b111000110010});
	TEST_ASSERT_EQUAL_VECTOR_HEX32(expected, dut.result());
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
