#include "compression/smol.hpp"

#include <array>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <format>
#include <iostream>
#include <numeric>
#include <map>
#include <stdexcept>
#include <string_view>
#include "subword_input_iterator.hpp"
#include "subword_output_iterator.hpp"
#include "compression/smol_tans.hpp"

static const bool PRINT_ENCODER_TANS(false);
static const bool PRINT_DECODER_TANS(false);
static const bool PRINT_DECODER_BITSTREAM(false);

enum class SymbolEncoding {
	IDENT,
	TANS,
	TANS_DELTA,
};

enum class LengthOffsetEncoding {
	VARINT,
	TANS,
};

class SmolMode {
private:
	char _code;
public:
	SmolMode(char code) : _code(code) {
		if (code < 1 || code > 2) {
			throw std::invalid_argument("SmolMode unknown code");
		}
	}

	char code() const {return this->_code;}

	char operator|(char rest) const {return this->_code | rest;}

	SymbolEncoding symbolEncoding() const {
		switch (this->_code) {
		case 1:
		case 4:
		case 8:
			return SymbolEncoding::IDENT;
		case 2:
		case 5:
			return SymbolEncoding::TANS;
		case 3:
		case 6:
			return SymbolEncoding::TANS_DELTA;
		}
		throw std::logic_error("SmolMode unknown code");
	}

	LengthOffsetEncoding lengthOffsetEncoding() const {
		switch (this->_code) {
		case 4:
		case 5:
		case 6:
			return LengthOffsetEncoding::TANS;
		case 1:
		case 2:
		case 3:
		case 8:
			return LengthOffsetEncoding::VARINT;
		}
		throw std::logic_error("SmolMode unknown code");
	}

	std::string_view name() const {
		switch (this->_code) {
		case 1:
			return "BASE_ONLY";
		case 2:
			return "ENCODE_SYMS";
		case 3:
			return "ENCODE_DELTA_SYMS";
		case 4:
			return "ENCODE_LO";
		case 5:
			return "ENCODE_BOTH";
		case 6:
			return "ENCODE_BOTH_DELTA_SYMS";
		case 8:
			return "TILEMAP";
		}
		return "UNKNOWN";
	}

	static const SmolMode BASE_ONLY;
	static const SmolMode ENCODE_SYMS;
};
const SmolMode SmolMode::BASE_ONLY(1);
const SmolMode SmolMode::ENCODE_SYMS(2);

std::ostream& operator<<(std::ostream& os, const SmolMode& mode) {
	return os << static_cast<int>(mode.code()) << " (" << mode.name() << ")";
}

class varint_input_iterator {
private:
	const std::vector<uint8_t>& _backing;
	size_t _byte_offset;

	static const unsigned CONTINUE_MASK = 0x80;
	static const unsigned DATA_MASK = 0x7F;
	static const unsigned DATA_SHIFT = 7;
public:
	varint_input_iterator(
		std::vector<uint8_t>& backing,
		size_t byte_offset) :
			_backing(backing),
			_byte_offset(byte_offset) {}

	~varint_input_iterator() {}

	unsigned operator*() const {
		unsigned retval = 0;
		size_t byte_offset = this->_byte_offset;
		unsigned shift = 0;

		retval = (this->_backing[byte_offset] & DATA_MASK);
		while (this->_backing[byte_offset] & CONTINUE_MASK) {
			++byte_offset;
			shift += DATA_SHIFT;
			retval |= (this->_backing[byte_offset] & DATA_MASK) << shift;
		}

		return retval;
	}

	varint_input_iterator& operator++() {
		while (this->_backing[this->_byte_offset] & CONTINUE_MASK) {
			++this->_byte_offset;
		}
		++this->_byte_offset;
		return *this;
	}

	varint_input_iterator operator++(int) {
		varint_input_iterator retval = *this;
		while (this->_backing[this->_byte_offset] & CONTINUE_MASK) {
			++this->_byte_offset;
		}
		++this->_byte_offset;
		return retval;
	}

	bool operator==(const varint_input_iterator& other) const {
		return
			this->_backing == other._backing &&
			this->_byte_offset == other._byte_offset;
	}

	bool operator!=(const varint_input_iterator& other) const {
		return !(*this == other);
	}

	bool operator<(const varint_input_iterator& other) const {
		return this->_byte_offset < other._byte_offset;
	}
};

std::vector<uint8_t> decompressSmol(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolMode mode(src[src_pos + 0] & 0xF);
	uint32_t imageSize = (src[src_pos + 0] >> 4) | (src[src_pos + 1] << 4) | ((src[src_pos + 2] & 0x3) << 12);
	uint32_t symbolsSize = (src[src_pos + 2] >> 2) | (src[src_pos + 3] << 6);
	uint32_t tansState = src[src_pos + 4] & 0x3F;
	uint32_t bitstreamSize = (src[src_pos + 4] >> 6) | (src[src_pos + 5] << 2) | ((src[src_pos + 6] & 0x7) << 10);
	uint32_t lengthoffsetSize = (src[src_pos + 6] >> 3) | (src[src_pos + 7] << 5);
	src_pos += 8;

	if (disassemble) {
		std::cout << std::endl;
		std::cout << "### Header " << std::endl;
		std::cout << "  mode: " << mode << std::endl;
		std::cout << "  imageSize: " << imageSize << std::endl;
		std::cout << "  symbolsSize: " << symbolsSize << std::endl;
		std::cout << "  initialTansState: " << tansState << std::endl;
		std::cout << "  bitstreamSize: " << bitstreamSize << std::endl;
		std::cout << "  lengthoffsetSize: " << lengthoffsetSize << std::endl;
		std::cout << std::endl;
	}

	std::array<DecodingTansCell, 64> tansTable;
	if (SymbolEncoding::TANS == mode.symbolEncoding()) {
		std::array<uint8_t, 16> frequencies;
		frequencies[15] = 0;
		for (unsigned i = 0; i < 3; i += 1, src_pos += 4) {
			uint32_t tmp = src[src_pos] | (src[src_pos+1] << 8) | (src[src_pos+2] << 16) | (src[src_pos+3] << 24);

			for (unsigned j = 0; j < 5; j++) {
				frequencies[i*5+j] = (tmp >> (6*j)) & 0x3F;
			}
			frequencies[15] |= ((tmp >> 30) & 0x3) << (2 * i);
		}

		if (disassemble) {
			std::cout << "### Symbol Frequencies " << std::endl;
			for (unsigned i = 0; i < frequencies.size(); i++) {
				std::cout << "  [" << i << "] = " << static_cast<int>(frequencies[i]) << std::endl;
			}
			std::cout << std::endl;
		}

		tansTable = genDecodingTansTable<16, 64>(frequencies);

		if (PRINT_DECODER_TANS && disassemble) {
			std::cout << "### Symbol tansTable " << std::endl;
			for (unsigned i = 0; i < tansTable.size(); i++) {
				std::cout
					<< std::format("  {:2d} ({:3d}) {:1x} {:3d} {:3d}",
						i, i + 64, tansTable[i].symbol, tansTable[i].next_state,
						tansTable[i].bits)
					<< std::endl;
			}
			std::cout << std::endl;
		}
	}

	std::vector<uint32_t> bitstream_words;
	bitstream_words.reserve(bitstreamSize);
	for (size_t i = 0; i < bitstreamSize; ++i, src_pos+=4) {
		uint32_t tmp = src[src_pos] | (src[src_pos+1] << 8) | (src[src_pos+2] << 16) | (src[src_pos+3] << 24);
		bitstream_words.push_back(tmp);
	}
	subword_input_iterator<uint32_t, uint1_t, DIRECTION_INC> bitstream(bitstream_words);

	std::vector<uint16_t> symbols;
	switch (mode.symbolEncoding()) {
		case SymbolEncoding::IDENT : {
			for (size_t i = 0; i < symbolsSize; i++) {
				symbols.push_back(src[src_pos + i * 2] | (src[src_pos + i * 2 + 1] << 8));
			}
			src_pos += symbolsSize * 2;
			break;
		}
		case SymbolEncoding::TANS : {
			if (PRINT_DECODER_BITSTREAM && disassemble) {
				std::cout << "### symbol bitstream " << std::endl;
			}
			for (size_t i = 0; i < symbolsSize; i++) {
				uint16_t symbol = 0;
				for (size_t j = 0; j < 4; j++) {
					DecodingTansCell column = tansTable[tansState];
					symbol |= column.symbol << (j * 4);
					tansState = column.next_state;
					unsigned offset = 0;
					for (unsigned k = 0; k < column.bits; ++k) {
						offset |= (*bitstream) << k;
						++bitstream;
					}
					tansState |= offset;
					tansState %= tansTable.size();
					if (PRINT_DECODER_BITSTREAM && disassemble) {
						for (unsigned bit_i = 0; bit_i < column.bits; bit_i++) {
							std::cout << (offset & (1 << bit_i) ? "1" : "0");
						}
						std::cout << "," << tansState << " ";
					}
				}
				symbols.push_back(symbol);
			}
			if (PRINT_DECODER_BITSTREAM && disassemble) {
				std::cout << std::endl;
			}
			break;
		}
		case SymbolEncoding::TANS_DELTA : {
			break;
		}
	}

	varint_input_iterator lengthOffsets(src, src_pos);
	varint_input_iterator lengthOffsetsEnd(src, src_pos + lengthoffsetSize);

	std::vector<uint16_t> retval16;
	size_t symbolIndex = 0;

	if (disassemble) {
		std::cout << "### Length-Offsets " << std::endl;
	}

	while (lengthOffsets < lengthOffsetsEnd) {
		unsigned length = *lengthOffsets;
		++lengthOffsets;
		unsigned offset = *lengthOffsets;
		++lengthOffsets;

		if (disassemble) {
			printf("  %8d %8d | ", length, offset);
		}

		if (0 == length) {
			for (unsigned j = 0; j < offset; j++) {
				if (disassemble) printf("%04x ", symbols[symbolIndex]);
				retval16.push_back(symbols[symbolIndex]);
				symbolIndex++;
			}
		} else {
			if (disassemble) printf("%04x # ", symbols[symbolIndex]);
			retval16.push_back(symbols[symbolIndex]);
			symbolIndex++;
			for (unsigned j = 0; j < length; j++) {
				if (false && disassemble) printf("%04x ", retval16[retval16.size() - offset]);
				retval16.push_back(retval16[retval16.size() - offset]);
			}
		}

		if (disassemble) {
			printf("\n");
		}
	}

	if (disassemble) {
		std::cout << "Final tansState: " << static_cast<int>(tansState) << std::endl;
	}

	std::vector<uint8_t> retval;
	for (uint16_t v : retval16) {
		retval.push_back(v);
		retval.push_back(v >> 8);
	}

	return retval;
}

struct SmolCopyInstruction1 {
	unsigned length;
	unsigned offset;
	uint16_t symbol;
};

class SmolCopyInstruction {
public:
	unsigned length;
	unsigned offset;
	std::vector<uint16_t> symbols;

	SmolCopyInstruction(unsigned _length, unsigned _offset, uint16_t _symbol) :
		length(_length), offset(_offset), symbols({_symbol}) {
		}

	SmolCopyInstruction(const std::vector<uint16_t>& _symbols) :
		length(0), offset(_symbols.size()), symbols(_symbols) {
			if (this->symbols.size() < 1)
				throw std::invalid_argument("symbols length (syms) = 0");
		}

	std::vector<uint8_t> varintBytes() const {
		std::vector<uint8_t> retval;
		if (this->length < 0x80) {
			retval.push_back(this->length);
		} else {
			retval.push_back((this->length & 0x7F) | 0x80);
			retval.push_back(this->length >> 7);
		}
		if (this->offset < 0x80) {
			retval.push_back(this->offset);
		} else {
			retval.push_back((this->offset & 0x7F) | 0x80);
			retval.push_back(this->offset >> 7);
		}
		return retval;
	}
};

static std::vector<SmolCopyInstruction> calculate_instructions(const std::vector<uint8_t> src) {
	static const unsigned max_offset = 0x3FFF;
	static const std::vector<unsigned char>::size_type max_length = 0x3FFF;
	static const std::vector<unsigned char>::size_type min_length = 2;

	std::vector<uint16_t> src16;
	for (size_t i = 0; i < src.size(); i += 2) {
		src16.push_back(src[i] | (src[i + 1] << 8));
	}

	std::vector<SmolCopyInstruction1> instr1s;
	size_t src_pos = 0;
	while (src_pos < src16.size()) {
		unsigned best_length = 0;
		unsigned best_offset = 0;
		uint16_t head_symbol = src16[src_pos];
		src_pos++;
		for (unsigned j = (src_pos > max_offset ? src_pos - max_offset : 0); j < src_pos; j++) {
			unsigned k;
			for (k = 0; k < std::min(max_length, src16.size() - src_pos); k++) {
				if (src16[src_pos + k] != src16[j + k]) {
					break;
				}
			}
			if (k >= best_length && k >= min_length) {
				best_length = k;
				best_offset = src_pos - j;
			}
		}
		instr1s.emplace_back(
			best_length,
			best_offset,
			head_symbol
		);
		src_pos += best_length;
	}

	std::vector<SmolCopyInstruction> instrs;
	size_t instr1_pos = 0;
	while (instr1_pos < instr1s.size()) {
		if (0 != instr1s[instr1_pos].offset) {
			instrs.emplace_back(
				instr1s[instr1_pos].length,
				instr1s[instr1_pos].offset,
				instr1s[instr1_pos].symbol
			);
			instr1_pos++;
		} else {
			std::vector<uint16_t> my_symbols({instr1s[instr1_pos].symbol});
			instr1_pos++;
			while (instr1_pos < instr1s.size() && 0 == instr1s[instr1_pos].offset) {
				my_symbols.push_back(instr1s[instr1_pos].symbol);
				instr1_pos++;
			}
			instrs.emplace_back(my_symbols);
		}
	}
	return instrs;
}

static std::vector<uint16_t> list_symbols(const std::vector<SmolCopyInstruction> instrs) {
	std::vector<uint16_t> retval;
	for (SmolCopyInstruction instr : instrs) {
		for (uint16_t symbol : instr.symbols) {
			retval.push_back(symbol);
		}
	}
	return retval;
}

static std::array<uint8_t, 16> calculate_normalized_symbol_frequencies(const std::vector<uint16_t> symbols) {
	std::array<unsigned, 16> symbolFrequencies_L{0};
	for (uint16_t symbol : symbols) {
		for (int i = 0; i < 4; i++) {
			symbolFrequencies_L[(symbol >> (4 * i)) & 0xF] += 1;
		}
	}

	std::array<uint8_t, 16> symbolFrequencies{0};
	for (unsigned i = 0; i < symbolFrequencies_L.size(); i++) {
		// `ceil` because we can't have symbol counts round down to zero
		symbolFrequencies[i] = std::ceil((symbolFrequencies_L[i] * (64.0 / 4)) / (symbols.size()));
	}

	unsigned symbolFrequencies_sum = std::accumulate(
			symbolFrequencies.begin(), symbolFrequencies.end(), 0);

	std::multimap<unsigned, unsigned> ordered_frequencies;
	for (unsigned i = 0; i < symbolFrequencies.size(); i++) {
		ordered_frequencies.emplace(symbolFrequencies[i], i);
	}

	std::multimap<unsigned, unsigned>::iterator ordered_frequencies_it = ordered_frequencies.begin();
	while (symbolFrequencies_sum > 64) {
		if (symbolFrequencies[ordered_frequencies_it->second] > 1) {
			symbolFrequencies[ordered_frequencies_it->second]--;
			symbolFrequencies_sum--;
		}
		++ordered_frequencies_it;
		if (ordered_frequencies_it == ordered_frequencies.end())
			ordered_frequencies_it = ordered_frequencies.begin();
	}
	while (symbolFrequencies_sum < 64) {
		++symbolFrequencies[ordered_frequencies_it->second];
		++symbolFrequencies_sum;

		++ordered_frequencies_it;
		if (ordered_frequencies_it == ordered_frequencies.end())
			ordered_frequencies_it = ordered_frequencies.begin();
	}

	for (unsigned i = 0; i < symbolFrequencies.size(); i++) {
		while (symbolFrequencies[i] >= 64) {
			symbolFrequencies[i]--;
			symbolFrequencies[(i + 1) % symbolFrequencies.size()]++;
		}
	}

	return symbolFrequencies;
}

static std::vector<uint1_t> calculate_reversed_tans_bitstream(unsigned& tansState, const std::vector<uint16_t> symbols, const std::array<uint8_t, 16> frequencies) {
	std::vector<uint1_t> retval;

	std::array<DecodingTansCell, 64> decodeTansTable = genDecodingTansTable<16, 64>(frequencies);
	std::array<std::array<EncodingTansCell, 16>, 64> encodeTansTable = genEncodingTansTable<16, 64>(decodeTansTable);

	for (auto i = symbols.rbegin(); i != symbols.rend(); ++i)
	{
		uint16_t symbol16 = *i;
		for (unsigned j = 0; j < 4; ++j) {
			uint8_t symbol = (symbol16 >> (4 * (3 - j))) & 0xF;

			EncodingTansCell ec = encodeTansTable[tansState][symbol];
			tansState = ec.next_state;
			for (unsigned bit_i = ec.bits; bit_i != 0; --bit_i)
			{
				uint1_t b = static_cast<uint1_t>(ec.value >> (bit_i - 1));
				if (PRINT_ENCODER_TANS) std::cout << (b ? "1" : "0");
				retval.push_back(b);
			}
			if (PRINT_ENCODER_TANS) std::cout << "," << static_cast<int>(ec.next_state) << " ";
		}
	}
	return retval;
}

static std::array<uint8_t, 12> pack_frequencies(std::array<uint8_t, 16> frequencies) {
	std::array<uint8_t, 12> retval;
	for (int i = 0; i < 3; ++i) {
		uint32_t packed = 0;
		for (int j = 0; j < 5; ++j) {
			packed |= frequencies[i * 5 + j] << (6 * j);
		}
		packed |= ((frequencies[15] >> (2 * i)) & 0x3) << 30;
		for (int j = 0; j < 4; j++) {
			retval[i * 4 + j] = packed >> (j * 8);
		}
	}
	return retval;
}

static std::optional<std::vector<uint8_t>> pack_header(
		SmolMode mode, size_t imageSize, size_t symbolsSize,
		uint32_t tansState, size_t bitstreamSize, size_t instructionsSize)
{
	if (imageSize >= 1 << 14)
		return std::nullopt;
	if (symbolsSize >= 1 << 14)
		return std::nullopt;
	if (tansState >= 1 << 6)
		return std::nullopt;
	if (bitstreamSize >= 1 << 13)
		return std::nullopt;
	if (instructionsSize >= 1 << 13)
		return std::nullopt;

	std::vector<uint8_t> retval;
	retval.push_back(mode | 0xF0);
	retval.push_back(imageSize * 4);
	retval.push_back((imageSize * 4) >> 8);
	retval.push_back((imageSize * 4) >> 16);

	retval.push_back(mode | imageSize << 4);
	retval.push_back(imageSize >> 4);
	retval.push_back(imageSize >> 12 | symbolsSize << 2);
	retval.push_back(symbolsSize >> 6);
	retval.push_back(tansState | bitstreamSize << 6);
	retval.push_back(bitstreamSize >> 2);
	retval.push_back(bitstreamSize >> 10 | instructionsSize << 3);
	retval.push_back(instructionsSize >> 5);
	return std::make_optional(retval);
}

std::optional<std::vector<uint8_t>> compressSmol1(std::vector<uint8_t> src) {
	if (0 != src.size() % 4)
		return std::nullopt;

	const SmolMode mode = SmolMode::BASE_ONLY;
	const uint32_t imageSize = src.size() / 4;
	const uint32_t tansState = 0;
	const uint32_t bitstreamSize = 0;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src);
	const std::vector<uint16_t> symbols = list_symbols(instrs);

	std::vector<uint8_t> instrBytes;
	for (SmolCopyInstruction instr : instrs) {
		for (uint8_t b : instr.varintBytes()) {
			instrBytes.push_back(b);
		}
	}

	std::optional<std::vector<uint8_t>> retval_opt = pack_header(
			mode, imageSize, symbols.size(), tansState, bitstreamSize, instrBytes.size());
	if (! retval_opt)
		return std::nullopt;
	std::vector<uint8_t> retval = *retval_opt;

	for (uint16_t symbol : symbols) {
		retval.push_back(symbol);
		retval.push_back(symbol >> 8);
	}

	for (uint8_t b : instrBytes) {
		retval.push_back(b);
	}

	while (0 != retval.size() % 4) {
		retval.push_back(0);
	}

	return std::make_optional(retval);
}

std::optional<std::vector<uint8_t>> compressSmol2(std::vector<uint8_t> src) {
	if (0 != src.size() % 4)
		return std::nullopt;

	const SmolMode mode = SmolMode::ENCODE_SYMS;
	uint32_t tansState = 0;

	const uint32_t imageSize = src.size() / 4;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src);
	const std::vector<uint16_t> symbols = list_symbols(instrs);
	const std::array<uint8_t, 16> symbolFrequencies = calculate_normalized_symbol_frequencies(symbols);

	// Want the start of the bitstream to be aligned with the beginning of the word.
	// Can't do that until knowing how many items are in the bitstream.
	std::vector<uint1_t> reversed_symbol_bitstream = calculate_reversed_tans_bitstream(tansState, symbols, symbolFrequencies);

	subword_output_iterator<uint32_t, uint1_t, DIRECTION_INC> symbol_bitstream;
	for (auto i = reversed_symbol_bitstream.rbegin(); i != reversed_symbol_bitstream.rend(); i++) {
		*symbol_bitstream = *i;
		++symbol_bitstream;
	}

	std::vector<uint8_t> instrBytes;
	for (SmolCopyInstruction instr : instrs) {
		for (uint8_t b : instr.varintBytes()) {
			instrBytes.push_back(b);
		}
	}

	std::optional<std::vector<uint8_t>> retval_opt = pack_header(
			mode, imageSize, symbols.size(), tansState, symbol_bitstream.size(), instrBytes.size());
	if (! retval_opt)
		return std::nullopt;
	std::vector<uint8_t> retval = *retval_opt;

	std::array<uint8_t, 12> packed_symbol_frequencies = pack_frequencies(symbolFrequencies);
	for (uint8_t b : packed_symbol_frequencies) {
		retval.push_back(b);
	}

	for (uint32_t symbol : symbol_bitstream.result()) {
		retval.push_back(symbol);
		retval.push_back(symbol >> 8);
		retval.push_back(symbol >> 16);
		retval.push_back(symbol >> 24);
	}

	for (uint8_t b : instrBytes) {
		retval.push_back(b);
	}

	while (0 != retval.size() % 4) {
		retval.push_back(0);
	}

	return std::make_optional(retval);
}
