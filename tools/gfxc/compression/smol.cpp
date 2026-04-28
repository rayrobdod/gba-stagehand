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
static const bool PRINT_DECODER_COPIES(false);

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
		if ((code < 1 || code > 6) && (code != 8)) {
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
	static const SmolMode ENCODE_SYMS_DELTA;
	static const SmolMode ENCODE_LO;
	static const SmolMode ENCODE_BOTH;
	static const SmolMode ENCODE_BOTH_DELTA;
	static const SmolMode TILEMAP;
};
const SmolMode SmolMode::BASE_ONLY(1);
const SmolMode SmolMode::ENCODE_SYMS(2);
const SmolMode SmolMode::ENCODE_SYMS_DELTA(3);
const SmolMode SmolMode::ENCODE_LO(4);
const SmolMode SmolMode::ENCODE_BOTH(5);
const SmolMode SmolMode::ENCODE_BOTH_DELTA(6);
const SmolMode SmolMode::TILEMAP(8);

std::ostream& operator<<(std::ostream& os, const SmolMode& mode) {
	return os << static_cast<int>(mode.code()) << " (" << mode.name() << ")";
}

class varint_input_iterator {
private:
	std::vector<uint8_t>::const_iterator _backing;

public:
	static const unsigned CONTINUE_MASK = 0x80;
	static const unsigned DATA_MASK = 0x7F;
	static const unsigned DATA_SHIFT = 7;
public:
	varint_input_iterator(
		std::vector<uint8_t>& backing,
		size_t byte_offset) :
			_backing(backing.begin() + byte_offset)
			{}

	~varint_input_iterator() {}

	unsigned operator*() const {
		unsigned retval = 0;
		std::vector<uint8_t>::const_iterator backing = this->_backing;

		retval = (*backing & DATA_MASK);
		if (*backing & CONTINUE_MASK) {
			++backing;
			retval |= *backing << DATA_SHIFT;
		}

		return retval;
	}

	varint_input_iterator& operator++() {
		if (*this->_backing & CONTINUE_MASK) {
			++this->_backing;
		}
		++this->_backing;
		return *this;
	}

	varint_input_iterator operator++(int) {
		varint_input_iterator retval = *this;
		if (*this->_backing & CONTINUE_MASK) {
			++this->_backing;
		}
		++this->_backing;
		return retval;
	}

	bool operator==(const varint_input_iterator& other) const {
		return
			this->_backing == other._backing;
	}

	bool operator!=(const varint_input_iterator& other) const {
		return !(*this == other);
	}

	bool operator<(const varint_input_iterator& other) const {
		return this->_backing < other._backing;
	}
};

class packing_input_iterator {
private:
	std::vector<uint8_t>::const_iterator _backing;
public:
	packing_input_iterator(const std::vector<uint8_t>::const_iterator& backing) :
		_backing(backing) {}
	packing_input_iterator(const packing_input_iterator& other) :
		_backing(other._backing) {}

	uint16_t operator*() {
		return (*_backing | (*(_backing + 1) << 8));
	}

	packing_input_iterator& operator++() {
		_backing += 2;
		return *this;
	}

	bool operator==(const packing_input_iterator& other) const {
		return this->_backing == other._backing;
	}
	bool operator!=(const packing_input_iterator& other) const {
		return !(*this == other);
	}
};

class tans_decoding_nibble_input_iterator {
private:
	const std::array<DecodingTansCell, 64> _table;
	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> _bitstream;
	unsigned _state;
	unsigned _zero_bit_advance_count;

public:
	tans_decoding_nibble_input_iterator(
		const std::array<DecodingTansCell, 64>& table,
		subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream,
		unsigned state,
		unsigned zero_bit_advance_count)
		: _table(table), _bitstream(bitstream), _state(state), _zero_bit_advance_count(zero_bit_advance_count) {}

	uint4_t operator*() {
		return static_cast<uint4_t>(this->_table[this->_state].symbol);
	}

	tans_decoding_nibble_input_iterator& operator++() {
		DecodingTansCell column = this->_table[this->_state];
		unsigned offset = 0;
		if (column.bits == 0) {
			_zero_bit_advance_count += 1;
		} else {
			for (unsigned k = 0; k < column.bits; ++k) {
				offset |= (*this->_bitstream) << k;
				++this->_bitstream;
			}
			_zero_bit_advance_count = 0;
		}
		this->_state = column.next_state;
		this->_state |= offset;
		this->_state %= this->_table.size();
		return *this;
	}

	bool operator<(tans_decoding_nibble_input_iterator& other) const {
		return this->_bitstream < other._bitstream || this->_zero_bit_advance_count < other._zero_bit_advance_count;
	}

	unsigned state() const { return this->_state; }
	unsigned zero_bit_advance_count() const { return this->_zero_bit_advance_count; }
	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream() const { return this->_bitstream; }
};

class tans_decoding_delta_nibble_input_iterator {
private:
	uint4_t _previous;
	tans_decoding_nibble_input_iterator _backing;
public:
	tans_decoding_delta_nibble_input_iterator(
		tans_decoding_nibble_input_iterator backing)
		: _previous(0), _backing(backing) {}

	tans_decoding_delta_nibble_input_iterator(
		const std::array<DecodingTansCell, 64>& table,
		subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream,
		unsigned state,
		unsigned zero_bit_advance_count)
		: _previous(0), _backing(table, bitstream, state, zero_bit_advance_count) {}

	uint4_t operator*() {
		return this->_previous + *(this->_backing);
	}

	tans_decoding_delta_nibble_input_iterator operator++() {
		this->_previous += *(this->_backing);
		++this->_backing;
		return *this;
	}

	unsigned state() const { return this->_backing.state(); }
	unsigned zero_bit_advance_count() const { return this->_backing.zero_bit_advance_count(); }
	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream() const { return this->_backing.bitstream(); }
};

class tans_decoding_u16_input_iterator {
private:
	tans_decoding_nibble_input_iterator _backing;
public:
	tans_decoding_u16_input_iterator(
		tans_decoding_nibble_input_iterator backing)
		: _backing(backing) {}

	tans_decoding_u16_input_iterator(
		const std::array<DecodingTansCell, 64>& table,
		subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream,
		unsigned state,
		unsigned zero_bit_advance_count)
		: _backing(table, bitstream, state, zero_bit_advance_count) {}

	uint16_t operator*() {
		tans_decoding_nibble_input_iterator backing = this->_backing;
		uint16_t symbol(0);
		for (unsigned j = 0; j < 4; j++) {
			symbol |= *backing << (j * 4);
			++backing;
		}
		return symbol;
	}

	tans_decoding_u16_input_iterator operator++() {
		++this->_backing;
		++this->_backing;
		++this->_backing;
		++this->_backing;
		return *this;
	}

	unsigned state() const { return this->_backing.state(); }
	unsigned zero_bit_advance_count() const { return this->_backing.zero_bit_advance_count(); }
	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream() const { return this->_backing.bitstream(); }
};

class tans_decoding_delta4_u16_input_iterator {
private:
	tans_decoding_delta_nibble_input_iterator _backing;
public:
	tans_decoding_delta4_u16_input_iterator(
		tans_decoding_delta_nibble_input_iterator backing)
		: _backing(backing) {}

	tans_decoding_delta4_u16_input_iterator(
		const std::array<DecodingTansCell, 64>& table,
		subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream,
		unsigned state,
		unsigned zero_bit_advance_count)
		: _backing(table, bitstream, state, zero_bit_advance_count) {}

	uint16_t operator*() {
		tans_decoding_delta_nibble_input_iterator backing = this->_backing;
		uint16_t symbol(0);
		for (unsigned j = 0; j < 4; j++) {
			symbol |= *backing << (j * 4);
			++backing;
		}
		return symbol;
	}

	tans_decoding_delta4_u16_input_iterator operator++() {
		++this->_backing;
		++this->_backing;
		++this->_backing;
		++this->_backing;
		return *this;
	}

	unsigned state() const { return this->_backing.state(); }
	unsigned zero_bit_advance_count() const { return this->_backing.zero_bit_advance_count(); }
	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream() const { return this->_backing.bitstream(); }
};

class tans_decoding_varint_input_iterator {
private:
	tans_decoding_nibble_input_iterator _backing;
public:
	tans_decoding_varint_input_iterator(
		tans_decoding_nibble_input_iterator backing)
		: _backing(backing) {}

	tans_decoding_varint_input_iterator(
		const std::array<DecodingTansCell, 64>& table,
		subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream,
		unsigned state,
		unsigned zero_bit_advance_count)
		: _backing(table, bitstream, state, zero_bit_advance_count) {}

	uint16_t operator*() {
		tans_decoding_nibble_input_iterator backing = this->_backing;
		uint16_t symbol(0);
		symbol |= *backing;
		++backing;
		symbol |= *backing << 4;
		++backing;
		if (symbol & varint_input_iterator::CONTINUE_MASK) {
			symbol &= varint_input_iterator::DATA_MASK;
			symbol |= *backing << varint_input_iterator::DATA_SHIFT;
			++backing;
			symbol |= *backing << (varint_input_iterator::DATA_SHIFT + 4);
			++backing;
		}
		return symbol;
	}

	tans_decoding_varint_input_iterator& operator++() {
		++this->_backing;
		if (*this->_backing & 0x8) {
			++this->_backing;
			++this->_backing;
		}
		++this->_backing;
		return *this;
	}

	bool operator<(tans_decoding_varint_input_iterator& other) {
		return this->_backing < other._backing;
	}

	tans_decoding_varint_input_iterator plus_backing_nibble_pairs(int n) {
		tans_decoding_nibble_input_iterator retval = this->_backing;
		for (int i = 0; i < n; ++i) {
			++retval;
			++retval;
		}
		return retval;
	}

	unsigned state() const { return this->_backing.state(); }
	unsigned zero_bit_advance_count() const { return this->_backing.zero_bit_advance_count(); }
	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream() const { return this->_backing.bitstream(); }
};

struct SmolHeader {
	SmolMode mode;
	uint32_t imageSize;
	uint32_t symbolsSize;
	uint32_t initialTansState;
	uint32_t bitstreamSize;
	uint32_t lengthOffsetSize;

	SmolHeader(const std::vector<uint8_t>& src, unsigned src_pos) : mode(1) {
		this->mode = src[src_pos + 0] & 0xF;
		this->imageSize = (src[src_pos + 0] >> 4) | (src[src_pos + 1] << 4) | ((src[src_pos + 2] & 0x3) << 12);
		this->symbolsSize = (src[src_pos + 2] >> 2) | (src[src_pos + 3] << 6);
		this->initialTansState = src[src_pos + 4] & 0x3F;
		this->bitstreamSize = (src[src_pos + 4] >> 6) | (src[src_pos + 5] << 2) | ((src[src_pos + 6] & 0x7) << 10);
		this->lengthOffsetSize = (src[src_pos + 6] >> 3) | (src[src_pos + 7] << 5);
	}
};

std::ostream& operator<<(std::ostream& os, const SmolHeader& h) {
	os << std::endl;
	os << "### Header " << std::endl;
	os << "  mode: " << h.mode << std::endl;
	os << "  imageSize: " << h.imageSize << std::endl;
	os << "  symbolsSize: " << h.symbolsSize << std::endl;
	os << "  initialTansState: " << h.initialTansState << std::endl;
	os << "  bitstreamSize: " << h.bitstreamSize << std::endl;
	os << "  lengthoffsetSize: " << h.lengthOffsetSize << std::endl;
	os << std::endl;
	return os;
}

struct SmolTilemapHeader {
	SmolMode mode;
	uint32_t imageSize;
	uint32_t symbolsSize;
	uint32_t lengthOffsetSize;

	SmolTilemapHeader(const std::vector<uint8_t>& src, unsigned src_pos) : mode(1) {
		this->mode = src[src_pos + 0] & 0xF;
		this->imageSize = (src[src_pos + 0] >> 4) | (src[src_pos + 1] << 4) | ((src[src_pos + 2] & 0x3) << 12);
		this->symbolsSize = (src[src_pos + 2] >> 2) | (src[src_pos + 3] << 6);
		// I believe lengthOffsetSize is still limited to 13 bits, but haven't verified.
		this->lengthOffsetSize = src[src_pos + 4] | (src[src_pos + 5] << 8) | (src[src_pos + 6] << 16) | (src[src_pos + 7] << 24);
	}
};

std::ostream& operator<<(std::ostream& os, const SmolTilemapHeader& h) {
	os << std::endl;
	os << "### Header " << std::endl;
	os << "  mode: " << h.mode << std::endl;
	os << "  imageSize: " << h.imageSize << std::endl;
	os << "  symbolsSize: " << h.symbolsSize << std::endl;
	os << "  lengthoffsetSize: " << h.lengthOffsetSize << std::endl;
	os << std::endl;
	return os;
}

std::array<DecodingTansCell, 64> unpack_frequencies_tans_table(const std::vector<uint8_t>& src, unsigned src_pos, bool disassemble) {
	std::array<DecodingTansCell, 64> tansTable;
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
	return tansTable;
}

template<class SYMBOL_ITERATOR, class INSTRUCTION_ITERATOR>
std::vector<uint8_t> decompress_smol_from_iterators(SYMBOL_ITERATOR symbols, INSTRUCTION_ITERATOR lengthOffsets, INSTRUCTION_ITERATOR lengthOffsetsEnd, bool disassemble) {
	std::vector<uint16_t> retval16;

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
				if (disassemble) printf("%04x ", *symbols);
				retval16.push_back(*symbols);
				++symbols;
			}
		} else {
			if (disassemble) printf("%04x # ", *symbols);
			retval16.push_back(*symbols);
			++symbols;
			for (unsigned j = 0; j < length; j++) {
				if (PRINT_DECODER_COPIES && disassemble) printf("%04x ", retval16[retval16.size() - offset]);
				retval16.push_back(retval16[retval16.size() - offset]);
			}
		}

		if (disassemble) {
			printf("\n");
		}
	}

	std::vector<uint8_t> retval;
	for (uint16_t v : retval16) {
		retval.push_back(v);
		retval.push_back(v >> 8);
	}

	return retval;
}

std::vector<uint8_t> decompressSmol1(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolHeader header(src, src_pos);
	src_pos += 8;
	if (disassemble) {
		std::cout << header;
	}

	packing_input_iterator symbols(src.begin() + src_pos);
	src_pos += header.symbolsSize * 2;

	varint_input_iterator lengthOffsets(src, src_pos);
	varint_input_iterator lengthOffsetsEnd(src, src_pos + header.lengthOffsetSize);

	return decompress_smol_from_iterators(symbols, lengthOffsets, lengthOffsetsEnd, disassemble);
}

std::vector<uint8_t> decompressSmol2(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolHeader header(src, src_pos);
	src_pos += 8;
	if (disassemble) {
		std::cout << header;
	}

	std::array<DecodingTansCell, 64> tansTable = unpack_frequencies_tans_table(src, src_pos, disassemble);
	src_pos += 12;

	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream(src.begin() + src_pos);
	src_pos += header.bitstreamSize * 4;

	tans_decoding_u16_input_iterator symbols(tansTable, bitstream, header.initialTansState, 0);

	varint_input_iterator lengthOffsets(src, src_pos);
	varint_input_iterator lengthOffsetsEnd(src, src_pos + header.lengthOffsetSize);

	return decompress_smol_from_iterators(symbols, lengthOffsets, lengthOffsetsEnd, disassemble);
}

std::vector<uint8_t> decompressSmol3(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolHeader header(src, src_pos);
	src_pos += 8;
	if (disassemble) {
		std::cout << header;
	}

	std::array<DecodingTansCell, 64> tansTable = unpack_frequencies_tans_table(src, src_pos, disassemble);
	src_pos += 12;

	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream(src.begin() + src_pos);
	src_pos += header.bitstreamSize * 4;

	tans_decoding_delta4_u16_input_iterator symbols(tansTable, bitstream, header.initialTansState, 0);

	varint_input_iterator lengthOffsets(src, src_pos);
	varint_input_iterator lengthOffsetsEnd(src, src_pos + header.lengthOffsetSize);

	return decompress_smol_from_iterators(symbols, lengthOffsets, lengthOffsetsEnd, disassemble);
}

std::vector<uint8_t> decompressSmol4(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolHeader header(src, src_pos);
	src_pos += 8;
	if (disassemble) {
		std::cout << header;
	}

	std::array<DecodingTansCell, 64> tansTable = unpack_frequencies_tans_table(src, src_pos, disassemble);
	src_pos += 12;

	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream(src.begin() + src_pos);
	src_pos += header.bitstreamSize * 4;

	packing_input_iterator symbols(src.begin() + src_pos);
	src_pos += header.symbolsSize * 2;

	tans_decoding_varint_input_iterator lengthOffsets(tansTable, bitstream, header.initialTansState, 0);
	tans_decoding_varint_input_iterator lengthOffsetsEnd = lengthOffsets.plus_backing_nibble_pairs(header.lengthOffsetSize);

	return decompress_smol_from_iterators(symbols, lengthOffsets, lengthOffsetsEnd, disassemble);
}

std::vector<uint8_t> decompressSmol5(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolHeader header(src, src_pos);
	src_pos += 8;
	if (disassemble) {
		std::cout << header;
	}

	std::array<DecodingTansCell, 64> loTansTable = unpack_frequencies_tans_table(src, src_pos, disassemble);
	src_pos += 12;
	std::array<DecodingTansCell, 64> symbolTansTable = unpack_frequencies_tans_table(src, src_pos, disassemble);
	src_pos += 12;

	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream(src.begin() + src_pos);

	tans_decoding_varint_input_iterator lengthOffsets(loTansTable, bitstream, header.initialTansState, 0);
	tans_decoding_varint_input_iterator lengthOffsetsEnd = lengthOffsets.plus_backing_nibble_pairs(header.lengthOffsetSize);

	tans_decoding_u16_input_iterator symbols(symbolTansTable, lengthOffsetsEnd.bitstream(), lengthOffsetsEnd.state(), lengthOffsetsEnd.zero_bit_advance_count());

	return decompress_smol_from_iterators(symbols, lengthOffsets, lengthOffsetsEnd, disassemble);
}

std::vector<uint8_t> decompressSmol6(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolHeader header(src, src_pos);
	src_pos += 8;
	if (disassemble) {
		std::cout << header;
	}

	std::array<DecodingTansCell, 64> loTansTable = unpack_frequencies_tans_table(src, src_pos, disassemble);
	src_pos += 12;
	std::array<DecodingTansCell, 64> symbolTansTable = unpack_frequencies_tans_table(src, src_pos, disassemble);
	src_pos += 12;

	subword_input_iterator<uint8_t, uint1_t, DIRECTION_INC> bitstream(src.begin() + src_pos);

	tans_decoding_varint_input_iterator lengthOffsets(loTansTable, bitstream, header.initialTansState, 0);
	tans_decoding_varint_input_iterator lengthOffsetsEnd = lengthOffsets.plus_backing_nibble_pairs(header.lengthOffsetSize);

	tans_decoding_delta4_u16_input_iterator symbols(symbolTansTable, lengthOffsetsEnd.bitstream(), lengthOffsetsEnd.state(), lengthOffsetsEnd.zero_bit_advance_count());

	return decompress_smol_from_iterators(symbols, lengthOffsets, lengthOffsetsEnd, disassemble);
}

std::vector<uint8_t> decompressSmol8(std::vector<uint8_t> src, bool disassemble) {
	static const unsigned WRAPPER_SIZE = 4;
	unsigned src_pos = WRAPPER_SIZE;

	SmolTilemapHeader header(src, src_pos);
	src_pos += 8;
	if (disassemble) {
		std::cout << header;
	}

	packing_input_iterator symbols(src.begin() + src_pos);
	src_pos += header.symbolsSize * 2;

	varint_input_iterator lengthOffsets(src, src_pos);
	varint_input_iterator lengthOffsetsEnd(src, src_pos + header.lengthOffsetSize);

	std::vector<uint8_t> deltas = decompress_smol_from_iterators(symbols, lengthOffsets, lengthOffsetsEnd, disassemble);

	std::vector<uint8_t> retval;
	uint16_t prev(0);
	for (auto i = deltas.begin(); i != deltas.end();) {
		uint16_t delta(0);
		delta |= *i++;
		delta |= (*i++) << 8;
		prev += delta;
		retval.push_back(prev);
		retval.push_back(prev >> 8);
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
			retval.push_back((this->length & varint_input_iterator::DATA_MASK) | varint_input_iterator::CONTINUE_MASK);
			retval.push_back(this->length >> varint_input_iterator::DATA_SHIFT);
		}
		if (this->offset < 0x80) {
			retval.push_back(this->offset);
		} else {
			retval.push_back((this->offset & varint_input_iterator::DATA_MASK) | varint_input_iterator::CONTINUE_MASK);
			retval.push_back(this->offset >> varint_input_iterator::DATA_SHIFT);
		}
		return retval;
	}
};

static std::vector<SmolCopyInstruction> calculate_instructions(
		const std::vector<uint8_t> src,
		const std::vector<unsigned char>::size_type min_length) {
	static const unsigned max_offset = 0x3FFF;
	static const std::vector<unsigned char>::size_type max_length = 0x3FFF;

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

template<class IT>
static std::array<uint8_t, 16> calculate_normalized_u4_frequencies(IT begin, IT end) {
	std::array<unsigned, 16> symbolFrequencies_L{0};
	for (IT i = begin; i != end; ++i) {
		symbolFrequencies_L[*i & 0xF] += 1;
	}

	unsigned symbolsCount = std::accumulate(
			symbolFrequencies_L.begin(), symbolFrequencies_L.end(), 0);

	std::array<uint8_t, 16> symbolFrequencies{0};
	for (unsigned i = 0; i < symbolFrequencies_L.size(); i++) {
		if (0 == symbolFrequencies_L[i]) {
			symbolFrequencies[i] = 0;
		} else {
			symbolFrequencies[i] = std::max<uint8_t>(1,
				std::round((symbolFrequencies_L[i] * 64.0) / symbolsCount));
		}
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

static std::vector<uint1_t> calculate_reversed_tans_bitstream_u4(unsigned& tansState, const std::vector<uint4_t> symbols, const std::array<uint8_t, 16> frequencies) {
	std::vector<uint1_t> retval;

	std::array<DecodingTansCell, 64> decodeTansTable = genDecodingTansTable<16, 64>(frequencies);
	std::array<std::array<EncodingTansCell, 16>, 64> encodeTansTable = genEncodingTansTable<16, 64>(decodeTansTable);

	for (auto i = symbols.rbegin(); i != symbols.rend(); ++i)
	{
		uint4_t symbol = *i;

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
	return retval;
}

static std::vector<uint1_t> calculate_reversed_tans_bitstream_u16(unsigned& tansState, const std::vector<uint16_t> symbols, const std::array<uint8_t, 16> frequencies) {
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
	if (src.size() >= 1 << 16)
		return std::nullopt;

	const SmolMode mode = SmolMode::BASE_ONLY;
	const uint32_t imageSize = src.size() / 4;
	const uint32_t tansState = 0;
	const uint32_t bitstreamSize = 0;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src, 2);
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
	if (src.size() >= 1 << 16)
		return std::nullopt;

	const SmolMode mode = SmolMode::ENCODE_SYMS;
	uint32_t tansState = 0;

	const uint32_t imageSize = src.size() / 4;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src, 4);

	const std::vector<uint16_t> symbols16 = list_symbols(instrs);
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4(symbols16.begin());
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4_end(symbols16.end());
	const std::array<uint8_t, 16> symbolFrequencies = calculate_normalized_u4_frequencies(symbols4, symbols4_end);

	// Want the start of the bitstream to be aligned with the beginning of the word.
	// Can't do that until knowing how many items are in the bitstream.
	std::vector<uint1_t> reversed_symbol_bitstream = calculate_reversed_tans_bitstream_u16(tansState, symbols16, symbolFrequencies);

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
			mode, imageSize, symbols16.size(), tansState, symbol_bitstream.size(), instrBytes.size());
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

std::optional<std::vector<uint8_t>> compressSmol3(std::vector<uint8_t> src) {
	if (0 != src.size() % 4)
		return std::nullopt;
	if (src.size() >= 1 << 16)
		return std::nullopt;

	const SmolMode mode = SmolMode::ENCODE_SYMS_DELTA;
	uint32_t tansState = 0;

	const uint32_t imageSize = src.size() / 4;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src, 4);

	const std::vector<uint16_t> symbols16 = list_symbols(instrs);
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4(symbols16.begin());
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4_end(symbols16.end());

	std::vector<uint4_t> symbolsDelta4;
	uint4_t previous(0);
	for (auto i = symbols4; i != symbols4_end; i++) {
		symbolsDelta4.push_back(*i - previous);
		previous = *i;
	}

	const std::array<uint8_t, 16> symbolFrequencies = calculate_normalized_u4_frequencies(symbolsDelta4.begin(), symbolsDelta4.end());

	// Want the start of the bitstream to be aligned with the beginning of the word.
	// Can't do that until knowing how many items are in the bitstream.
	std::vector<uint1_t> reversed_symbol_bitstream = calculate_reversed_tans_bitstream_u4(tansState, symbolsDelta4, symbolFrequencies);

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
			mode, imageSize, symbols16.size(), tansState, symbol_bitstream.size(), instrBytes.size());
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

std::optional<std::vector<uint8_t>> compressSmol4(std::vector<uint8_t> src) {
	if (0 != src.size() % 4)
		return std::nullopt;
	if (src.size() >= 1 << 16)
		return std::nullopt;

	const SmolMode mode = SmolMode::ENCODE_LO;
	uint32_t tansState = 0;

	const uint32_t imageSize = src.size() / 4;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src, 1);
	const std::vector<uint16_t> symbols = list_symbols(instrs);

	std::vector<uint4_t> instrNibbles;
	for (SmolCopyInstruction instr : instrs) {
		for (uint8_t b : instr.varintBytes()) {
			instrNibbles.push_back(static_cast<uint4_t>(b));
			instrNibbles.push_back(static_cast<uint4_t>(b >> 4));
		}
	}
	const std::array<uint8_t, 16> instrFrequencies = calculate_normalized_u4_frequencies(instrNibbles.begin(), instrNibbles.end());

	// Want the start of the bitstream to be aligned with the beginning of the word.
	// Can't do that until knowing how many items are in the bitstream.
	std::vector<uint1_t> reversed_instr_bitstream = calculate_reversed_tans_bitstream_u4(tansState, instrNibbles, instrFrequencies);

	subword_output_iterator<uint32_t, uint1_t, DIRECTION_INC> instr_bitstream;
	for (auto i = reversed_instr_bitstream.rbegin(); i != reversed_instr_bitstream.rend(); i++) {
		*instr_bitstream = *i;
		++instr_bitstream;
	}


	std::optional<std::vector<uint8_t>> retval_opt = pack_header(
			mode, imageSize, symbols.size(), tansState, instr_bitstream.size(), instrNibbles.size() / 2);
	if (! retval_opt)
		return std::nullopt;
	std::vector<uint8_t> retval = *retval_opt;

	std::array<uint8_t, 12> packed_instr_frequencies = pack_frequencies(instrFrequencies);
	for (uint8_t b : packed_instr_frequencies) {
		retval.push_back(b);
	}

	for (uint32_t symbol : instr_bitstream.result()) {
		retval.push_back(symbol);
		retval.push_back(symbol >> 8);
		retval.push_back(symbol >> 16);
		retval.push_back(symbol >> 24);
	}

	for (uint16_t symbol : symbols) {
		retval.push_back(symbol);
		retval.push_back(symbol >> 8);
	}

	while (0 != retval.size() % 4) {
		retval.push_back(0);
	}

	return std::make_optional(retval);
}

std::optional<std::vector<uint8_t>> compressSmol5(std::vector<uint8_t> src) {
	if (0 != src.size() % 4)
		return std::nullopt;
	if (src.size() >= 1 << 16)
		return std::nullopt;

	const SmolMode mode = SmolMode::ENCODE_BOTH;
	uint32_t tansState = 0;

	const uint32_t imageSize = src.size() / 4;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src, 2);
	const std::vector<uint16_t> symbols = list_symbols(instrs);

	std::vector<uint4_t> instrNibbles;
	for (SmolCopyInstruction instr : instrs) {
		for (uint8_t b : instr.varintBytes()) {
			instrNibbles.push_back(static_cast<uint4_t>(b));
			instrNibbles.push_back(static_cast<uint4_t>(b >> 4));
		}
	}
	const std::array<uint8_t, 16> instrFrequencies = calculate_normalized_u4_frequencies(instrNibbles.begin(), instrNibbles.end());

	const std::vector<uint16_t> symbols16 = list_symbols(instrs);
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4(symbols16.begin());
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4_end(symbols16.end());
	const std::array<uint8_t, 16> symbolFrequencies = calculate_normalized_u4_frequencies(symbols4, symbols4_end);

	// Want the start of the bitstream to be aligned with the beginning of the word.
	// Can't do that until knowing how many items are in the bitstream.
	std::vector<uint1_t> reversed_sym_bitstream = calculate_reversed_tans_bitstream_u16(tansState, symbols16, symbolFrequencies);
	std::vector<uint1_t> reversed_instr_bitstream = calculate_reversed_tans_bitstream_u4(tansState, instrNibbles, instrFrequencies);

	subword_output_iterator<uint32_t, uint1_t, DIRECTION_INC> bitstream;
	for (auto i = reversed_instr_bitstream.rbegin(); i != reversed_instr_bitstream.rend(); i++) {
		*bitstream = *i;
		++bitstream;
	}
	for (auto i = reversed_sym_bitstream.rbegin(); i != reversed_sym_bitstream.rend(); i++) {
		*bitstream = *i;
		++bitstream;
	}


	std::optional<std::vector<uint8_t>> retval_opt = pack_header(
			mode, imageSize, symbols.size(), tansState, bitstream.size(), instrNibbles.size() / 2);
	if (! retval_opt)
		return std::nullopt;
	std::vector<uint8_t> retval = *retval_opt;

	std::array<uint8_t, 12> packed_instr_frequencies = pack_frequencies(instrFrequencies);
	for (uint8_t b : packed_instr_frequencies) {
		retval.push_back(b);
	}

	std::array<uint8_t, 12> packed_symbol_frequencies = pack_frequencies(symbolFrequencies);
	for (uint8_t b : packed_symbol_frequencies) {
		retval.push_back(b);
	}

	for (uint32_t symbol : bitstream.result()) {
		retval.push_back(symbol);
		retval.push_back(symbol >> 8);
		retval.push_back(symbol >> 16);
		retval.push_back(symbol >> 24);
	}

	return std::make_optional(retval);
}

std::optional<std::vector<uint8_t>> compressSmol6(std::vector<uint8_t> src) {
	if (0 != src.size() % 4)
		return std::nullopt;
	if (src.size() >= 1 << 16)
		return std::nullopt;

	const SmolMode mode = SmolMode::ENCODE_BOTH_DELTA;
	uint32_t tansState = 0;

	const uint32_t imageSize = src.size() / 4;

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(src, 2);
	const std::vector<uint16_t> symbols = list_symbols(instrs);

	std::vector<uint4_t> instrNibbles;
	for (SmolCopyInstruction instr : instrs) {
		for (uint8_t b : instr.varintBytes()) {
			instrNibbles.push_back(static_cast<uint4_t>(b));
			instrNibbles.push_back(static_cast<uint4_t>(b >> 4));
		}
	}
	const std::array<uint8_t, 16> instrFrequencies = calculate_normalized_u4_frequencies(instrNibbles.begin(), instrNibbles.end());

	const std::vector<uint16_t> symbols16 = list_symbols(instrs);
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4(symbols16.begin());
	const subword_input_iterator<uint16_t, uint4_t, DIRECTION_INC> symbols4_end(symbols16.end());

	std::vector<uint4_t> symbolsDelta4;
	uint4_t previous(0);
	for (auto i = symbols4; i != symbols4_end; i++) {
		symbolsDelta4.push_back(*i - previous);
		previous = *i;
	}

	const std::array<uint8_t, 16> symbolFrequencies = calculate_normalized_u4_frequencies(symbolsDelta4.begin(), symbolsDelta4.end());

	// Want the start of the bitstream to be aligned with the beginning of the word.
	// Can't do that until knowing how many items are in the bitstream.
	std::vector<uint1_t> reversed_sym_bitstream = calculate_reversed_tans_bitstream_u4(tansState, symbolsDelta4, symbolFrequencies);
	std::vector<uint1_t> reversed_instr_bitstream = calculate_reversed_tans_bitstream_u4(tansState, instrNibbles, instrFrequencies);

	subword_output_iterator<uint32_t, uint1_t, DIRECTION_INC> bitstream;
	for (auto i = reversed_instr_bitstream.rbegin(); i != reversed_instr_bitstream.rend(); i++) {
		*bitstream = *i;
		++bitstream;
	}
	for (auto i = reversed_sym_bitstream.rbegin(); i != reversed_sym_bitstream.rend(); i++) {
		*bitstream = *i;
		++bitstream;
	}


	std::optional<std::vector<uint8_t>> retval_opt = pack_header(
			mode, imageSize, symbols.size(), tansState, bitstream.size(), instrNibbles.size() / 2);
	if (! retval_opt)
		return std::nullopt;
	std::vector<uint8_t> retval = *retval_opt;

	std::array<uint8_t, 12> packed_instr_frequencies = pack_frequencies(instrFrequencies);
	for (uint8_t b : packed_instr_frequencies) {
		retval.push_back(b);
	}

	std::array<uint8_t, 12> packed_symbol_frequencies = pack_frequencies(symbolFrequencies);
	for (uint8_t b : packed_symbol_frequencies) {
		retval.push_back(b);
	}

	for (uint32_t symbol : bitstream.result()) {
		retval.push_back(symbol);
		retval.push_back(symbol >> 8);
		retval.push_back(symbol >> 16);
		retval.push_back(symbol >> 24);
	}

	return std::make_optional(retval);
}

std::optional<std::vector<uint8_t>> compressSmol8(std::vector<uint8_t> src) {
	if (0 != src.size() % 4)
		return std::nullopt;
	if (src.size() >= 1 << 16)
		return std::nullopt;

	const SmolMode mode = SmolMode::TILEMAP;
	const uint32_t imageSize = src.size() / 4;

	std::vector<uint8_t> deltas;
	uint16_t prev(0);
	for (auto i = src.begin(); i != src.end();) {
		uint16_t current(0);
		current |= *i++;
		current |= (*i++) << 8;
		uint16_t delta = current - prev;
		prev = current;
		deltas.push_back(delta);
		deltas.push_back(delta >> 8);
	}

	const std::vector<SmolCopyInstruction> instrs = calculate_instructions(deltas, 2);
	const std::vector<uint16_t> symbols = list_symbols(instrs);

	std::vector<uint8_t> instrBytes;
	for (SmolCopyInstruction instr : instrs) {
		for (uint8_t b : instr.varintBytes()) {
			instrBytes.push_back(b);
		}
	}

	if (imageSize >= 1 << 14)
		return std::nullopt;
	if (symbols.size() >= 1 << 14)
		return std::nullopt;
	if (instrBytes.size() >= 1 << 13)
		return std::nullopt;

	std::vector<uint8_t> retval;
	retval.push_back(mode | 0xF0);
	retval.push_back(imageSize * 4);
	retval.push_back((imageSize * 4) >> 8);
	retval.push_back((imageSize * 4) >> 16);

	retval.push_back(mode | imageSize << 4);
	retval.push_back(imageSize >> 4);
	retval.push_back(imageSize >> 12 | symbols.size() << 2);
	retval.push_back(symbols.size() >> 6);
	retval.push_back(instrBytes.size());
	retval.push_back(instrBytes.size() >> 8);
	retval.push_back(instrBytes.size() >> 16);
	retval.push_back(instrBytes.size() >> 24);


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
