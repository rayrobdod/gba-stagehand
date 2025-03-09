#include <array>
#include <cstdio>
#include "compression/huff.cpp"
#include "test/_shared.hpp"

void test_decompress_Huff_3(void) {
	const std::vector<uint8_t> input{0x28, 4, 0, 0, 3, 0x80, 'f', 0xC0, 'H', 'u', 0, 0, 0, 0, 0, 0b10110000};
	std::vector<uint8_t> result = decompressHuff8(input, false);

	const std::vector<uint8_t> expected{'H', 'u', 'f', 'f'};
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_decompress_Huff_4(void) {
	const std::vector<uint8_t> input{0x28, 4, 0, 0, 4, 0x80, 'f', 0xC0, 'H', 'u', 0, 0, 0, 0b10110000, 0, 0};
	std::vector<uint8_t> result = decompressHuff8(input, false);

	const std::vector<uint8_t> expected{'H', 'u', 'f', 'f'};
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_decompress_Hex_1(void) {
	const std::vector<uint8_t> input{
		0x28, 32, 0, 0,
		1,
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0xC3, 0xC4, 0xC4, 0xC5, 0xC5, 0xC6, 0xC6, 0xC7,
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
	};
	std::vector<uint8_t> result = decompressHuff8(input, false);

	const uint8_t expected_[33] = "03020201C5C4C4C3C7C6C6C533323130";
	const std::vector<uint8_t> expected(expected_, expected_ + 32);
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_decompress_Hex_2(void) {
	const std::vector<uint8_t> input{
		0x28, 32, 0, 0,
		2,
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0xC3, 0xC4, 0xC4, 0xC5, 0xC5, 0xC6, 0xC6, 0xC7,
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
	};
	std::vector<uint8_t> result = decompressHuff8(input, false);

	const uint8_t expected_[33] = "02010302C4C3C5C4C6C5C7C631303332";
	const std::vector<uint8_t> expected(expected_, expected_ + 32);

	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_compress8_Huff(void) {
	const std::vector<uint8_t> input{'H', 'u', 'f', 'f'};
	std::vector<uint8_t> result = compressHuff8(input);

	const std::vector<uint8_t> expected{0x28, 4, 0, 0, 3, 0x80, 'f', 0xC0, 'H', 'u', 0, 0, 0, 0, 0, 0b10110000};
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_compress8_Digits(void) {
	const std::vector<uint8_t> input{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
	std::vector<uint8_t> result = compressHuff8(input);

	const std::vector<uint8_t> expected{0x28, 8, 0, 0, 7, 0, 0, 1, 0xC1, 0xC2, 0xC2, 0xC3, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0, 0x77, 0x39, 0x5};
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

void test_make_prefix_tree_Huff(void) {
	char frequencies[256] = {0};
	frequencies['H'] = 1;
	frequencies['u'] = 1;
	frequencies['f'] = 2;

	prefix_tree result = make_prefix_tree_from_frequencies(frequencies, frequencies + 256);

	//std::cout << std::endl << result << std::endl;

	TEST_ASSERT_EQUAL_UNSIGNED(2, result.code('H').length);
	TEST_ASSERT_EQUAL_UNSIGNED(2, result.code('u').length);
	TEST_ASSERT_EQUAL_UNSIGNED(1, result.code('f').length);
}

int main(int argc, char** argv) {
	total = 0;
	failed = 0;

	RUN_TEST(test_decompress_Huff_3);
	RUN_TEST(test_decompress_Huff_4);
	RUN_TEST(test_decompress_Hex_1);
	RUN_TEST(test_decompress_Hex_2);

	RUN_TEST(test_make_prefix_tree_Huff);

	RUN_TEST(test_compress8_Huff);
	RUN_TEST(test_compress8_Digits);

	printf("Total: %d; Failing: %d\n", total, failed);
	return 0 != failed;
}
