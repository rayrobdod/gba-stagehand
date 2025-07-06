#include "compression/rl.hpp"

#include <cstdio>

static const unsigned max_run = 0xFF;

std::vector<uint8_t> decompressRlZero(std::vector<uint8_t> src, bool disassemble) {
	unsigned expected_size = src[1] | src[2] << 8 | src[3] << 16;
	std::vector<uint8_t> dest;

	unsigned srcPos = 4;

	while (dest.size() < expected_size) {
		if (disassemble)
			printf("  %8d %8zd | %3d %3d\n", srcPos, dest.size(), src[srcPos], src[srcPos+1]);
		unsigned zero_count = src[srcPos++];
		for (unsigned i = 0; i < zero_count; i++) {
			dest.push_back(0);
			dest.push_back(0);
		}
		unsigned copy_count = src[srcPos++];
		for (unsigned i = 0; i < copy_count; i++) {
			dest.push_back(src[srcPos++]);
			dest.push_back(src[srcPos++]);
		}
	}
	if (disassemble)
		printf("  %8d %8zd | END\n\n", srcPos, dest.size());
	return dest;
}

std::optional<std::vector<uint8_t>> compressRlZero(std::vector<uint8_t> src) {
	if (0 != src.size() % 2)
		return std::nullopt;

	std::vector<uint8_t> result;
	result.push_back(0x31);
	result.push_back(src.size());
	result.push_back(src.size() >> 8);
	result.push_back(src.size() >> 16);

	unsigned src_pos = 0;

	while (src_pos < src.size()) {
		unsigned zero_count = 0;

		while (src_pos + 2 <= src.size() && zero_count < max_run
				&& src[src_pos] == 0 && src[src_pos + 1] == 0) {
			zero_count++;
			src_pos += 2;
		}

		result.push_back(zero_count);

		const unsigned copy_count_pos = result.size();
		result.push_back(0);
		unsigned copy_count = 0;

		while (src_pos + 2 <= src.size() && copy_count < max_run
				&& (src[src_pos] != 0 || src[src_pos + 1] != 0
					|| (src_pos + 4 <= src.size() && (src[src_pos + 2] != 0 || src[src_pos + 3] != 0))
				)
			) {
			copy_count++;
			result.push_back(src[src_pos++]);
			result.push_back(src[src_pos++]);
		}

		result[copy_count_pos] = copy_count;
	}

	while (result.size() % 4 != 0) {
		result.push_back(0);
	}

	return std::make_optional(result);
}
