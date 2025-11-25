#include <cstdint>
#include <optional>
#include <vector>

std::optional<std::vector<uint8_t>> compressSmol1(std::vector<uint8_t> src);
std::optional<std::vector<uint8_t>> compressSmol2(std::vector<uint8_t> src);
std::vector<uint8_t> decompressSmol(std::vector<uint8_t> src, bool disassemble);
