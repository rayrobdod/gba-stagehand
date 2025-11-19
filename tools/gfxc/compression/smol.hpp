#include <cstdint>
#include <optional>
#include <vector>

enum CompressionMode {
	BASE_ONLY = 1,
	ENCODE_SYMS = 2,
	ENCODE_DELTA_SYMS = 3,
	ENCODE_LO = 4,
	ENCODE_BOTH = 5,
	ENCODE_BOTH_DELTA_SYMS = 6,
	IS_FRAME_CONTAINER = 7,
	IS_TILEMAP = 8,
};

std::optional<std::vector<uint8_t>> compressSmol(std::vector<uint8_t> src);
std::vector<uint8_t> decompressSmol(std::vector<uint8_t> src, bool disassemble);
