#include <cstdint>
#include <optional>
#include <vector>

std::optional<std::vector<uint8_t>> compressRl(std::vector<uint8_t> src);
std::vector<uint8_t> decompressRl(std::vector<uint8_t> src, bool disassemble);
