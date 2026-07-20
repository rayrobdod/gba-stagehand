#include "compression/lz.hpp"

#include <cstdio>

struct LzCopyInstruction {
	unsigned length;
	union {
		unsigned offset;
		unsigned value;
	};
};

std::vector<uint8_t> decompressLz(std::vector<uint8_t> src, bool disassemble) {
	unsigned expected_size = src[1] | src[2] << 8 | src[3] << 16;
	std::vector<uint8_t> dest(expected_size);

	static const std::vector<uint8_t>::size_type flags_per_byte = 8;

	unsigned destPos = 0;
	unsigned srcPos = 4;
	unsigned flagPos = flags_per_byte;
	uint8_t flag;

	while (destPos < expected_size) {
		if (flagPos >= flags_per_byte) {
			flag = src[srcPos++];
			flagPos = 0;
		}

		if (flag & (1 << (flags_per_byte - 1 - flagPos))) {
			if (disassemble)
				printf("  %8d %8d | ", srcPos, destPos);
			unsigned opcode = src[srcPos + 1] | (src[srcPos] << 8);
			srcPos += 2;
			unsigned width = (opcode >> 12) + 3;
			unsigned distance = (opcode & 0xFFF) + 1;
			if (disassemble)
				printf("COPY: %2d %d\n", width, distance);

			if (destPos + width <= expected_size) {
				for (unsigned i = 0; i < width; i++) {
					dest[destPos] = dest[destPos - distance];
					destPos++;
				}
			} else {
				destPos += width;
			}
		} else {
			if (disassemble)
				printf("  %8d %8d | IMM : %02X\n", srcPos, destPos, src[srcPos]);
			dest[destPos++] = src[srcPos++];
		}
		flagPos++;
	}
	if (disassemble)
		printf("  %8d %8d | END\n\n", srcPos, destPos);
	return dest;
}

std::vector<uint8_t> decompressLz11(std::vector<uint8_t> src, bool disassemble) {
	unsigned expected_size = src[1] | src[2] << 8 | src[3] << 16;
	std::vector<uint8_t> dest(expected_size);

	static const std::vector<uint8_t>::size_type flags_per_byte = 8;

	unsigned destPos = 0;
	unsigned srcPos = 4;
	unsigned flagPos = flags_per_byte;
	uint8_t flag;

	while (destPos < expected_size) {
		if (flagPos >= flags_per_byte) {
			flag = src[srcPos++];
			flagPos = 0;
		}

		if (flag & (1 << (flags_per_byte - 1 - flagPos))) {
			if (disassemble)
				printf("  %8d %8d | ", srcPos, destPos);

			unsigned width;
			unsigned distance;

			switch (src[srcPos] >> 4) {
			case 0:
				width = ((src[srcPos] << 4) | ((src[srcPos + 1] >> 4) & 0xF)) + 0x11;
				distance = (((src[srcPos + 1] & 0xF) << 8) | (src[srcPos + 2])) + 1;
				srcPos += 3;
				break;
			case 1:
				width = (((src[srcPos] & 0xF) << 12) | (src[srcPos + 1] << 4) | ((src[srcPos + 2] >> 4) & 0xF)) + 0x111;
				distance = (((src[srcPos + 2] & 0xF) << 8) | (src[srcPos + 3])) + 1;
				srcPos += 4;
				break;
			default:
				width = (src[srcPos] >> 4) + 1;
				distance = (((src[srcPos] & 0xF) << 8) | (src[srcPos + 1])) + 1;
				srcPos += 2;
				break;
			}

			if (disassemble)
				printf("COPY: %2d %d\n", width, distance);

			if (destPos + width <= expected_size) {
				for (unsigned i = 0; i < width; i++) {
					dest[destPos] = dest[destPos - distance];
					destPos++;
				}
			} else {
				destPos += width;
			}
		} else {
			if (disassemble)
				printf("  %8d %8d | IMM : %02X\n", srcPos, destPos, src[srcPos]);
			dest[destPos++] = src[srcPos++];
		}
		flagPos++;
	}
	if (disassemble)
		printf("  %8d %8d | END\n\n", srcPos, destPos);
	return dest;
}

std::vector<uint8_t> decompressLz16(std::vector<uint8_t> src, bool disassemble) {
	unsigned expected_size = src[1] | src[2] << 8 | src[3] << 16;
	std::vector<uint8_t> dest(expected_size);

	static const std::vector<uint8_t>::size_type flags_per_byte = 16;

	unsigned destPos = 0;
	unsigned srcPos = 4;
	unsigned flagPos = flags_per_byte;
	uint16_t flag;

	while (destPos < expected_size) {
		if (flagPos >= flags_per_byte) {
			flag = src[srcPos++];
			flag |= src[srcPos++] << 8;
			flagPos = 0;
		}

		if (flag & (1 << (flags_per_byte - 1 - flagPos))) {
			if (disassemble)
				printf("  %8d %8d | ", srcPos, destPos);
			unsigned opcode = src[srcPos] | (src[srcPos + 1] << 8);
			srcPos += 2;
			unsigned width = ((opcode >> 12) + 3) * 2;
			unsigned distance = ((opcode & 0xFFF) + 1) * 2;
			if (disassemble)
				printf("COPY: %2d %d\n", width, distance);

			if (destPos + width <= expected_size) {
				for (unsigned i = 0; i < width; i++) {
					dest[destPos] = dest[destPos - distance];
					destPos++;
				}
			} else {
				destPos += width;
			}
		} else {
			if (disassemble)
				printf("  %8d %8d | IMM : %02X\n", srcPos, destPos, src[srcPos]);
			dest[destPos++] = src[srcPos++];
			dest[destPos++] = src[srcPos++];
		}
		flagPos++;
	}
	if (disassemble)
		printf("  %8d %8d | END\n\n", srcPos, destPos);
	return dest;
}

template <typename word>
static std::vector<LzCopyInstruction> instructions(std::vector<word> src, typename std::vector<word>::size_type max_length) {
	static const unsigned max_offset = 0xFFF;
	static const typename std::vector<word>::size_type min_length = 3;

	std::vector<LzCopyInstruction> instrs;

	if (src.size() == 0)
		return instrs;

	unsigned src_pos = 1;

	instrs.push_back((LzCopyInstruction){.length = 1, .value = src[0]});
	while (src_pos < src.size()) {
		LzCopyInstruction best;
		best.length = 1;
		best.value = src[src_pos];
		// GBA Bios LzUncompVram doesn't write correctly with offset = 1
		for (unsigned j = (src_pos > max_offset ? src_pos - max_offset : 0); j < src_pos - 1; j++) {
			unsigned k;
			for (k = 0; k < std::min(max_length, src.size() - src_pos); k++) {
				if (src[src_pos + k] != src[j + k]) {
					break;
				}
			}
			if (k >= best.length && k >= min_length) {
				best.length = k;
				best.offset = src_pos - j - 1;
			}
		}
		instrs.push_back(best);
		src_pos += best.length;
	}

	return instrs;
}

std::optional<std::vector<uint8_t>> compressLz(std::vector<uint8_t> src) {
	auto instrs = instructions<uint8_t>(src, 15 + 3);

	std::vector<uint8_t> result;
	result.push_back(0x10);
	result.push_back(src.size());
	result.push_back(src.size() >> 8);
	result.push_back(src.size() >> 16);

	static const std::vector<uint8_t>::size_type flags_per_byte = 8;

	for (unsigned i = 0; i < instrs.size(); i += flags_per_byte) {
		uint8_t flags(0);
		for (unsigned j = 0; j < std::min(flags_per_byte, instrs.size() - i); j++) {
			if (instrs[i + j].length != 1)
				flags |= 1 << (flags_per_byte - 1 - j);
		}
		result.push_back(flags);

		for (unsigned j = 0; j < std::min(flags_per_byte, instrs.size() - i); j++) {
			if (instrs[i + j].length == 1) {
				result.push_back(instrs[i + j].value);
			} else {
				result.push_back(((instrs[i + j].length - 3) << 4) | (instrs[i + j].offset >> 8));
				result.push_back(instrs[i + j].offset);
			}
		}
	}

	while (result.size() % 4 != 0) {
		result.push_back(0);
	}

	return std::make_optional(result);
}

std::optional<std::vector<uint8_t>> compressLz11(std::vector<uint8_t> src) {
	auto instrs = instructions<uint8_t>(src, 0xFFFF + 0x111);

	std::vector<uint8_t> result;
	result.push_back(0x11);
	result.push_back(src.size());
	result.push_back(src.size() >> 8);
	result.push_back(src.size() >> 16);

	static const std::vector<uint8_t>::size_type flags_per_byte = 8;

	for (unsigned i = 0; i < instrs.size(); i += flags_per_byte) {
		uint8_t flags(0);
		for (unsigned j = 0; j < std::min(flags_per_byte, instrs.size() - i); j++) {
			if (instrs[i + j].length != 1)
				flags |= 1 << (flags_per_byte - 1 - j);
		}
		result.push_back(flags);

		for (unsigned j = 0; j < std::min(flags_per_byte, instrs.size() - i); j++) {
			if (instrs[i + j].length == 1) {
				result.push_back(instrs[i + j].value);
			} else {
				if (instrs[i + j].length <= (0xF + 0x1)) {
					result.push_back(((instrs[i + j].length - 1) << 4) | (instrs[i + j].offset >> 8));
					result.push_back(instrs[i + j].offset);
				} else if (instrs[i + j].length <= (0xFF + 0x11)) {
					unsigned length = instrs[i + j].length - 0x11;
					result.push_back((length >> 4));
					result.push_back((length << 4) | (instrs[i + j].offset >> 8));
					result.push_back(instrs[i + j].offset);
				} else {
					unsigned length = instrs[i + j].length - 0x111;
					result.push_back(0x10 | length >> 12);
					result.push_back(length >> 4);
					result.push_back((length << 4) | (instrs[i + j].offset >> 8));
					result.push_back(instrs[i + j].offset);
				}
			}
		}
	}

	while (result.size() % 4 != 0) {
		result.push_back(0);
	}

	return std::make_optional(result);
}

std::optional<std::vector<uint8_t>> compressLz16(std::vector<uint8_t> src) {
	if (0 != src.size() % 2)
		return std::nullopt;

	std::vector<uint16_t> src16;
	for (std::vector<unsigned char>::size_type i = 0; i < src.size(); i += 2) {
		src16.push_back(src[i] | (src[i + 1] << 8));
	}
	auto instrs = instructions<uint16_t>(src16, 15 + 3);

	std::vector<uint8_t> result;
	result.push_back(0x16);
	result.push_back(src.size());
	result.push_back(src.size() >> 8);
	result.push_back(src.size() >> 16);

	static const std::vector<uint16_t>::size_type flags_per_byte = 16;

	for (unsigned i = 0; i < instrs.size(); i += flags_per_byte) {
		uint16_t flags(0);
		for (unsigned j = 0; j < std::min(flags_per_byte, instrs.size() - i); j++) {
			if (instrs[i + j].length != 1)
				flags |= 1 << (flags_per_byte - 1 - j);
		}
		result.push_back(flags);
		result.push_back(flags >> 8);

		for (unsigned j = 0; j < std::min(flags_per_byte, instrs.size() - i); j++) {
			if (instrs[i + j].length == 1) {
				result.push_back(instrs[i + j].value);
				result.push_back(instrs[i + j].value >> 8);
			} else {
				uint16_t opcode = 0;
				opcode |= (instrs[i + j].length - 3) << 12;
				opcode |= instrs[i + j].offset;
				result.push_back(opcode);
				result.push_back(opcode >> 8);
			}
		}
	}

	while (result.size() % 4 != 0) {
		result.push_back(0);
	}

	return std::make_optional(result);
}
