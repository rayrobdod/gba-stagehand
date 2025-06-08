#include <cstdio>
#include <string>
#include "compression/frit.hpp"
#include "test/_shared.hpp"

void test_frit8_Zeros_32(void) {
	const std::vector<uint8_t> input(32);
	std::vector<uint8_t> result = compressFrit8(input);

	const std::vector<uint8_t> expected{
		0x41, 0x20, 0, 0,
		0x8F, 1, 0, 0
	};
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_frit16_Zeros_32(void) {
	const std::vector<uint8_t> input(32);
	std::vector<uint8_t> result = compressFrit16(input);

	const std::vector<uint8_t> expected{
		0x42, 0x20, 0, 0,
		0x8E, 0x80, 0, 0
	};
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_frit8_sixteen_loop(void) {
	const std::string input_s("abcdefghijklmnopabcdefghijklmnop");
	const std::vector<uint8_t> input(input_s.begin(), input_s.end());
	std::vector<uint8_t> result = compressFrit8(input);

	const std::vector<uint8_t> expected{
		0x41, 0x20, 0x0, 0x0,
		0x0D, 'a', 0xFE, 0xF0,
		0x09, 'a', 0xEE, 0xE0,
	};
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

int main(int argc, char** argv) {
	total = 0;
	failed = 0;

	RUN_TEST(test_frit8_Zeros_32);
	RUN_TEST(test_frit16_Zeros_32);
	RUN_TEST(test_frit8_sixteen_loop);

	printf("Total: %d; Failing: %d\n", total, failed);
	return 0 != failed;
}
