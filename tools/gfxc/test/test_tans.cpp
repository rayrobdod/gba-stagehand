#include "compression/smol_tans.hpp"

#include <cstdio>
#include "test/_shared.hpp"

bool operator==(const EncodingTansCell& lhs, const EncodingTansCell& rhs) {
	return
		lhs.next_state == rhs.next_state &&
		lhs.bits == rhs.bits &&
		lhs.value == rhs.value &&
		true;
}

template<size_t SYMBOL_COUNT, size_t FREQUENCY_SUM>
void printEncodingTansTable(std::array<std::array<EncodingTansCell, SYMBOL_COUNT>, FREQUENCY_SUM> table) {
	printf("\n");
	for (unsigned j = 0; j < SYMBOL_COUNT; ++j) {
		for (unsigned i = 0; i < FREQUENCY_SUM; ++i) {
			EncodingTansCell cell = table[i][j];
			printf("  %2d,%2d,%2d  ", cell.next_state, cell.bits, cell.value);
		}
		printf("\n");
	}
	printf("\n");
};

// https://medium.com/@bredelet/understanding-ans-coding-through-examples-d1bebfc7e076
void test_1(void) {
	const std::array<DecodingTansCell, 8> input{
		(DecodingTansCell){.symbol = 1, .bits = 2, .next_state = 3 << 2},
		(DecodingTansCell){.symbol = 0, .bits = 1, .next_state = 5 << 1},
		(DecodingTansCell){.symbol = 2, .bits = 2, .next_state = 2 << 2},
		(DecodingTansCell){.symbol = 2, .bits = 2, .next_state = 3 << 2},
		(DecodingTansCell){.symbol = 1, .bits = 1, .next_state = 4 << 1},
		(DecodingTansCell){.symbol = 0, .bits = 2, .next_state = 3 << 2},
		(DecodingTansCell){.symbol = 0, .bits = 1, .next_state = 4 << 1},
		(DecodingTansCell){.symbol = 1, .bits = 1, .next_state = 5 << 1},
	};

	const std::array<std::array<EncodingTansCell, 3>, 8> expected{
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state = 14 % 8, .bits = 1, .value = 0},
			(EncodingTansCell){.next_state = 12 % 8, .bits = 1, .value = 0},
			(EncodingTansCell){.next_state = 10 % 8, .bits = 2, .value = 0},
		},
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state = 14 % 8, .bits = 1, .value = 1},
			(EncodingTansCell){.next_state = 12 % 8, .bits = 1, .value = 1},
			(EncodingTansCell){.next_state = 10 % 8, .bits = 2, .value = 1},
		},
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state =  9 % 8, .bits = 1, .value = 0},
			(EncodingTansCell){.next_state = 15 % 8, .bits = 1, .value = 0},
			(EncodingTansCell){.next_state = 10 % 8, .bits = 2, .value = 2},
		},
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state =  9 % 8, .bits = 1, .value = 1},
			(EncodingTansCell){.next_state = 15 % 8, .bits = 1, .value = 1},
			(EncodingTansCell){.next_state = 10 % 8, .bits = 2, .value = 3},
		},
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state = 13 % 8, .bits = 2, .value = 0},
			(EncodingTansCell){.next_state =  8 % 8, .bits = 2, .value = 0},
			(EncodingTansCell){.next_state = 11 % 8, .bits = 2, .value = 0},
		},
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state = 13 % 8, .bits = 2, .value = 1},
			(EncodingTansCell){.next_state =  8 % 8, .bits = 2, .value = 1},
			(EncodingTansCell){.next_state = 11 % 8, .bits = 2, .value = 1},
		},
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state = 13 % 8, .bits = 2, .value = 2},
			(EncodingTansCell){.next_state =  8 % 8, .bits = 2, .value = 2},
			(EncodingTansCell){.next_state = 11 % 8, .bits = 2, .value = 2},
		},
		(std::array<EncodingTansCell, 3>){
			(EncodingTansCell){.next_state = 13 % 8, .bits = 2, .value = 3},
			(EncodingTansCell){.next_state =  8 % 8, .bits = 2, .value = 3},
			(EncodingTansCell){.next_state = 11 % 8, .bits = 2, .value = 3},
		},
	};

	auto result = genEncodingTansTable<3, 8>(input);

	if (false) {
		printEncodingTansTable(expected);
		printEncodingTansTable(result);
	}

	if (result != expected) {
		TEST_FAIL("actual != expected");
	}
}


int main() {
	total = 0;
	failed = 0;

	RUN_TEST(test_1);

	printf("Total: %d; Failing: %d\n", total, failed);
	return 0 != failed;
}
