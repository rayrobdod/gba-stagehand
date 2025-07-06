#include <cstdint>
#include <optional>
#include <vector>

std::optional<std::vector<uint8_t>> compressRlZero(std::vector<uint8_t> src);
std::vector<uint8_t> decompressRlZero(std::vector<uint8_t> src, bool disassemble);
