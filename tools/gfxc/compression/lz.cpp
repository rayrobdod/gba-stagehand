#include "compression/lz.hpp"

#include <cstdio>

struct CopyInstruction {
	unsigned length;
	union {
		unsigned offset;
		unsigned value;
	};
};

static unsigned max_offset = 0xFFF;
static std::vector<uint8_t>::size_type max_length = 15 + 3;
static std::vector<uint8_t>::size_type min_length = 3;
static std::vector<uint8_t>::size_type flags_per_byte = 8;

std::vector<uint8_t> decompressLz(std::vector<uint8_t> src, bool decompile) {
	unsigned expected_size = src[1] | src[2] << 8 | src[3] << 16;
	std::vector<uint8_t> dest(expected_size);

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
			if (decompile)
				printf("  %8d %8d | ", srcPos, destPos);
			unsigned opcode = src[srcPos + 1] | (src[srcPos] << 8);
			srcPos += 2;
			unsigned width = (opcode >> 12) + 3;
			unsigned distance = (opcode & 0xFFF) + 1;
			if (decompile)
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
			if (decompile)
				printf("  %8d %8d | IMM : %02X\n", srcPos, destPos, src[srcPos]);
			dest[destPos++] = src[srcPos++];
		}
		flagPos++;
	}
	if (decompile)
		printf("  %8d %8d | END\n\n", srcPos, destPos);
	return dest;
}

std::vector<uint8_t> compressLz(std::vector<uint8_t> src) {
	std::vector<CopyInstruction> src_instrs;
	src_instrs.push_back((CopyInstruction){.length = 1, .value = src[0]});
	for (unsigned i = 1; i < src.size(); i++) {
		CopyInstruction best;
		best.length = 1;
		best.value = src[i];
		// GBA Bios LzUncompVram doesn't write correctly with offset = 1
		for (unsigned j = (i > max_offset ? i - max_offset : 0); j < i - 1; j++) {
			unsigned k;
			for (k = 0; k < std::min(max_length, src.size() - i); k++) {
				if (src[i + k] != src[j + k]) {
					break;
				}
			}
			if (k >= best.length && k >= min_length) {
				best.length = k;
				best.offset = i - j - 1;
			}
		}
		src_instrs.push_back(best);
	}

	std::vector<CopyInstruction> dest_instrs;
	unsigned srcPos = 0;
	while (srcPos < src.size()) {
		dest_instrs.push_back(src_instrs[srcPos]);
		srcPos += src_instrs[srcPos].length;
	}

	std::vector<uint8_t> result;
	result.push_back(0x10);
	result.push_back(src.size());
	result.push_back(src.size() >> 8);
	result.push_back(src.size() >> 16);

	for (unsigned i = 0; i < dest_instrs.size(); i += flags_per_byte) {
		uint8_t flags(0);
		for (unsigned j = 0; j < std::min(flags_per_byte, dest_instrs.size() - i); j++) {
			if (dest_instrs[i + j].length != 1)
				flags |= 1 << (flags_per_byte - 1 - j);
		}
		result.push_back(flags);

		for (unsigned j = 0; j < std::min(flags_per_byte, dest_instrs.size() - i); j++) {
			if (dest_instrs[i + j].length == 1) {
				result.push_back(dest_instrs[i + j].value);
			} else {
				result.push_back(((dest_instrs[i + j].length - 3) << 4) | (dest_instrs[i + j].offset >> 8));
				result.push_back(dest_instrs[i + j].offset);
			}
		}
	}

	while (result.size() % 4 != 0) {
		result.push_back(0);
	}

	return result;
}