#include "compression/smol.hpp"

#include <cstdio>
#include <iostream>

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

	uint32_t mode = src[WRAPPER_SIZE + 0] & 0xF;
	uint32_t imageSize = (src[WRAPPER_SIZE + 0] >> 4) | (src[WRAPPER_SIZE + 1] << 4) | ((src[WRAPPER_SIZE + 2] & 0x3) << 12);
	uint32_t symbolsSize = (src[WRAPPER_SIZE + 2] >> 2) | (src[WRAPPER_SIZE + 3] << 6);
	uint32_t initialState = src[WRAPPER_SIZE + 4] & 0x3F;
	uint32_t bitstreamSize = (src[WRAPPER_SIZE + 4] >> 6) | (src[WRAPPER_SIZE + 5] << 2) | ((src[WRAPPER_SIZE + 6] & 0x7) << 10);
	uint32_t lengthoffsetSize = (src[WRAPPER_SIZE + 6] >> 3) | (src[WRAPPER_SIZE + 7] << 5);

	if (disassemble) {
		std::cout << std::endl;
		std::cout << "### Header " << std::endl;
		std::cout << "  mode: " << mode << std::endl;
		std::cout << "  imageSize: " << imageSize << std::endl;
		std::cout << "  symbolsSize: " << symbolsSize << std::endl;
		std::cout << "  initialState: " << initialState << std::endl;
		std::cout << "  bitstreamSize: " << bitstreamSize << std::endl;
		std::cout << "  lengthoffsetSize: " << lengthoffsetSize << std::endl;
	}

	std::vector<uint16_t> symbols;
	for (size_t i = 0; i < symbolsSize; i++) {
		symbols.push_back(src[WRAPPER_SIZE + 8 + i * 2] | (src[WRAPPER_SIZE + 8 + i * 2 + 1] << 8));
	}

	varint_input_iterator lengthOffsets(src, WRAPPER_SIZE + 8 + symbolsSize * 2);
	varint_input_iterator lengthOffsetsEnd(src, WRAPPER_SIZE + 8 + symbolsSize * 2 + lengthoffsetSize);

	std::vector<uint16_t> retval16;
	size_t symbolIndex = 0;

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

std::optional<std::vector<uint8_t>> compressSmol(std::vector<uint8_t> src) {
	static const unsigned max_offset = 0x3FFF;
	static const std::vector<unsigned char>::size_type max_length = 0x3FFF;
	static const std::vector<unsigned char>::size_type min_length = 2;

	const enum CompressionMode mode = BASE_ONLY;
	const uint32_t initialState = 0;
	const uint32_t bitstreamSize = 0;

	if (0 != src.size() % 4)
		return std::nullopt;
	const uint32_t imageSize = src.size() / 4;

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

	std::vector<uint16_t> symbols;
	for (SmolCopyInstruction instr : instrs) {
		for (uint16_t symbol : instr.symbols) {
			symbols.push_back(symbol);
		}
	}

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
	if (initialState >= 1 << 6)
		return std::nullopt;
	if (bitstreamSize >= 1 << 13)
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
	retval.push_back(initialState | bitstreamSize << 6);
	retval.push_back(bitstreamSize >> 2);
	retval.push_back(bitstreamSize >> 10 | instrBytes.size() << 3);
	retval.push_back(instrBytes.size() >> 5);

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
