
#include <array>
#include <string>
#include <numeric>
#include <cstdint>
#include <stdexcept>

struct DecodingTansCell {
	uint8_t symbol;
	uint8_t bits;
	uint8_t next_state;
};

struct EncodingTansCell {
	uint8_t next_state;
	uint8_t bits;
	uint8_t value;
};

template<size_t SYMBOL_COUNT, size_t FREQUENCY_SUM>
std::array<DecodingTansCell, FREQUENCY_SUM> genDecodingTansTable(std::array<uint8_t, SYMBOL_COUNT> frequencies) {
	static_assert(1 == __builtin_popcount(FREQUENCY_SUM));

	unsigned frequency_sum = std::accumulate<typename std::array<uint8_t, SYMBOL_COUNT>::iterator, unsigned>(frequencies.begin(), frequencies.end(), 0);
	if (frequency_sum != FREQUENCY_SUM) {
		std::string msg("frequencies did not sum to ");
		msg += std::to_string(FREQUENCY_SUM);
		msg += "; Was: ";
		msg += std::to_string(frequency_sum);
		throw std::logic_error(msg);
	}

	std::array<DecodingTansCell, FREQUENCY_SUM> retval;
	size_t tableIndex = 0;
	for (unsigned i = 0; i < frequencies.size(); i++) {
		for (unsigned j = 0; j < frequencies[i]; j++, tableIndex++) {
			retval[tableIndex].symbol = i;
			retval[tableIndex].next_state = frequencies[i] + j;
			retval[tableIndex].bits = __builtin_clz(frequencies[i] + j) - __builtin_clz(retval.size());
			retval[tableIndex].next_state <<= retval[tableIndex].bits;
		}
	}
	return retval;
}

template<size_t SYMBOL_COUNT, size_t FREQUENCY_SUM>
std::array<std::array<EncodingTansCell, SYMBOL_COUNT>, FREQUENCY_SUM> genEncodingTansTable(std::array<DecodingTansCell, FREQUENCY_SUM> decodingTansTable) {
	static_assert(1 == __builtin_popcount(FREQUENCY_SUM));

	std::array<std::array<EncodingTansCell, SYMBOL_COUNT>, FREQUENCY_SUM> retval{0};
	for (uint8_t j = 0; j < decodingTansTable.size(); ++j) {
		DecodingTansCell decodingColumn = decodingTansTable[j];
		EncodingTansCell ec{
			.next_state = j,
			.bits = decodingColumn.bits,
			.value = 0,
		};

		for (int i = 0; i < (1 << decodingColumn.bits); ++i) {
			ec.value = i;
			retval[(decodingColumn.next_state + i) % FREQUENCY_SUM][decodingColumn.symbol] = ec;
		}
	}

	return retval;
}
