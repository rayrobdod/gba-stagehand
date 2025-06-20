#include <cstdint>
#include <optional>
#include <vector>

std::optional<std::vector<uint8_t>> compressHuff4(std::vector<uint8_t> src);
std::vector<uint8_t> decompressHuff4(std::vector<uint8_t> src, bool disassemble);

std::optional<std::vector<uint8_t>> compressHuff8(std::vector<uint8_t> src);
std::vector<uint8_t> decompressHuff8(std::vector<uint8_t> src, bool disassemble);
