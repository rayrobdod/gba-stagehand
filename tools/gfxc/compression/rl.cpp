#include "compression/rl.hpp"

#include <cstdio>

static const unsigned max_repeat_run = 0x7F + 3;
static const unsigned max_raw_run = 0x7F + 1;

std::vector<uint8_t> decompressRl(std::vector<uint8_t> src, bool decompile) {
	unsigned expected_size = src[1] | src[2] << 8 | src[3] << 16;
	std::vector<uint8_t> dest(expected_size);

	unsigned destPos = 0;
	unsigned srcPos = 4;

	while (destPos < expected_size) {
		uint8_t flags = src[srcPos++];

		if (0x80 & flags) {
			unsigned width = (flags & 0x7F) + 3;
			uint8_t value = src[srcPos++];
			if (decompile)
				printf("  %8d %8d | RUN %d %d\n\n", srcPos, destPos, width, value);

			if (destPos + width <= expected_size) {
				for (unsigned i = 0; i < width; i++) {
					dest[destPos++] = value;
				}
			} else {
				destPos += width;
			}
		} else {
			unsigned width = (flags & 0x7F) + 1;
			if (decompile)
				printf("  %8d %8d | LEN %d \n\n", srcPos, destPos, width);

			if (destPos + width <= expected_size) {
				for (unsigned i = 0; i < width; i++) {
					dest[destPos++] = src[srcPos++];
				}
			} else {
				destPos += width;
			}
		}
	}
	if (decompile)
		printf("  %8d %8d | END\n\n", srcPos, destPos);
	return dest;
}

std::vector<uint8_t> compressRl(std::vector<uint8_t> src) {
	std::vector<uint8_t> result;
	result.push_back(0x30);
	result.push_back(src.size());
	result.push_back(src.size() >> 8);
	result.push_back(src.size() >> 16);

	unsigned src_pos = 0;

	while (src_pos < src.size()) {
		if (src_pos + 3 < src.size()) {
			if (src[src_pos] == src[src_pos + 1] && src[src_pos] == src[src_pos + 2]) {
				unsigned i = 2;
				while (src_pos + i < src.size() && i < max_repeat_run && src[src_pos] == src[src_pos + i]) {
					i++;
				}
				result.push_back(0x80 | (i - 3));
				result.push_back(src[src_pos]);
				src_pos += i;
			} else {
				unsigned flag_pos = result.size();
				result.push_back(0);
				unsigned len = 0;
				while (src_pos < src.size() && len < max_raw_run && (src_pos + 2 >= src.size() ||
						src[src_pos] != src[src_pos + 1] || src[src_pos] != src[src_pos + 2])) {
					result.push_back(src[src_pos++]);
					len++;
				}
				result[flag_pos] = len - 1;
			}
		} else {
			result.push_back(src.size() - src_pos - 1);
			while (src_pos < src.size()) {
				result.push_back(src[src_pos++]);
			}
		}
	}

	while (result.size() % 4 != 0) {
		result.push_back(0);
	}

	return result;
}
