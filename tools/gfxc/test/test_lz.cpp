#include <cstdio>
#include <string>
#include "compression/lz.hpp"
#include "test/_shared.hpp"

void test_compress10_Zeros_32(void) {
	const std::vector<uint8_t> input(32);
	std::optional<std::vector<uint8_t>> result_opt = compressLz(input);

	const std::vector<uint8_t> expected{
		0x10, 0x20, 0, 0,
		0x30, 0, 0, 0xF0, 0x01, 0x90, 0x01, 0
	};

	if (result_opt) {
		std::vector<uint8_t> result = *result_opt;
		TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
	} else {
		TEST_FAIL("Compression failed");
	}
}

void test_compress11_Zeros_32(void) {
	const std::vector<uint8_t> input(32);
	std::optional<std::vector<uint8_t>> result_opt = compressLz11(input);

	const std::vector<uint8_t> expected{
		0x11, 0x20, 0, 0,
		0x20, 0, 0, 0x00, 0xD0, 0x01, 0, 0
	};

	if (result_opt) {
		std::vector<uint8_t> result = *result_opt;
		TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
	} else {
		TEST_FAIL("Compression failed");
	}
}

void test_compress11_sixteen_loop(void) {
	const std::string input_s("abcdefghijklmnopabcdefghijklmnop");
	const std::vector<uint8_t> input(input_s.begin(), input_s.end());
	std::optional<std::vector<uint8_t>> result_opt = compressLz11(input);

	const std::vector<uint8_t> expected{
		0x11, 0x20, 0x0, 0x0,
		0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
		0x00, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
		0x80, 0xF0, 0x0F, 0x0, 0x0, 0x0
	};

	if (result_opt) {
		std::vector<uint8_t> result = *result_opt;
		TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
	} else {
		TEST_FAIL("Compression failed");
	}
}

void test_compress16_Zeros_64(void) {
	const std::vector<uint8_t> input(64);
	std::optional<std::vector<uint8_t>> result_opt = compressLz16(input);

	const std::vector<uint8_t> expected{
		0x16, 0x40, 0, 0,
		0, 0x30, 0, 0, 0, 0, 0x01, 0xF0, 0x01, 0x90, 0, 0
	};

	if (result_opt) {
		std::vector<uint8_t> result = *result_opt;
		TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
	} else {
		TEST_FAIL("Compression failed");
	}
}

void test_decompress16_Zeros_64(void) {
	const std::vector<uint8_t> input{
		0x16, 0x40, 0, 0,
		0, 0x30, 0, 0, 0, 0, 0x01, 0xF0, 0x01, 0x90, 0, 0
	};
	std::vector<uint8_t> result = decompressLz16(input, false);

	const std::vector<uint8_t> expected(64);

	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, result);
}

int main() {
	total = 0;
	failed = 0;

	RUN_TEST(test_compress10_Zeros_32);
	RUN_TEST(test_compress11_Zeros_32);
	RUN_TEST(test_compress11_sixteen_loop);
	RUN_TEST(test_compress16_Zeros_64);
	RUN_TEST(test_decompress16_Zeros_64);

	printf("Total: %d; Failing: %d\n", total, failed);
	return 0 != failed;
}
