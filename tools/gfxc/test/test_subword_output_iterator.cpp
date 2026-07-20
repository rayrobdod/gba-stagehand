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

void test_assignment_does_not_advance(void) {
	subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> dut;
	*dut = 2_u4;
	*dut = 3_u4;
	*dut = 5_u4;
	*dut = 7_u4;
	std::vector<uint8_t> expected({0x7});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}

void test_plus_eq(void) {
	subword_output_iterator<uint8_t, uint2_t, DIRECTION_INC> dut;
	*dut = 3_u2;
	dut += 6;
	*dut = 3_u2;
	std::vector<uint8_t> expected({0b00000011, 0b00110000});
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected, dut.result());
}

void test_plus_eq_one_is_same_as_increment(void) {
	subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> dut;
	*dut = 7_u4;
	dut += 1;
	*dut = 13_u4;
	subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> expected;
	*expected = 7_u4;
	expected++;
	*expected = 13_u4;
	TEST_ASSERT_EQUAL_VECTOR_HEX8(expected.result(), dut.result());
}

int main() {
	total = 0;
	failed = 0;

	RUN_TEST(test_4bpp_10);
	RUN_TEST(test_4bpp_16);
	RUN_TEST(test_2bpp_16);
	RUN_TEST(test_1bpp);
	RUN_TEST(test_assignment_does_not_advance);
	RUN_TEST(test_plus_eq);
	RUN_TEST(test_plus_eq_one_is_same_as_increment);

	printf("Total: %d; Failing: %d\n", total, failed);
	return 0 != failed;
}
