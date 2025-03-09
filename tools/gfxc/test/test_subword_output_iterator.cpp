#include <cstdio>
#include "test/_shared.hpp"
#include "subword_output_iterator.hpp"

void test_4bpp_10(void) {
	subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> dut;
	for (unsigned i = 0; i < 10; i++) {
		uint4_t i2(i);
		*(dut++) = i2;
	}
	std::vector<uint8_t> expected({0x10, 0x32, 0x54, 0x76, 0x98});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}
void test_4bpp_16(void) {
	subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> dut;
	for (unsigned i = 0; i < 16; i++) {
		uint4_t i2(i);
		*(dut++) = i2;
	}
	std::vector<uint8_t> expected({0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}
void test_2bpp_16(void) {
	subword_output_iterator<uint8_t, uint2_t, DIRECTION_INC> dut;
	for (unsigned i = 0; i < 16; i++) {
		uint2_t i2(i);
		*(dut++) = i2;
	}
	std::vector<uint8_t> expected({0xE4, 0xE4, 0xE4, 0xE4});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}
void test_1bpp(void) {
	subword_output_iterator<uint8_t, uint1_t, DIRECTION_INC> dut;
	*(dut++) = 0_u1;
	*(dut++) = 1_u1;
	*(dut++) = 0_u1;
	*(dut++) = 0_u1;
	*(dut++) = 1_u1;
	*(dut++) = 1_u1;
	*(dut++) = 0_u1;
	*(dut++) = 0_u1;
	*(dut++) = 0_u1;
	*(dut++) = 1_u1;
	*(dut++) = 1_u1;
	*(dut++) = 1_u1;
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
