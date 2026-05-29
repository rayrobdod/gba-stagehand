#include <cstdint>
#include <optional>
#include <vector>

std::optional<std::vector<uint8_t>> compressLz(std::vector<uint8_t> src);
std::vector<uint8_t> decompressLz(std::vector<uint8_t> src, bool disassemble);

std::optional<std::vector<uint8_t>> compressLz11(std::vector<uint8_t> src);
std::vector<uint8_t> decompressLz11(std::vector<uint8_t> src, bool disassemble);

std::optional<std::vector<uint8_t>> compressLz16(std::vector<uint8_t> src);
std::vector<uint8_t> decompressLz16(std::vector<uint8_t> src, bool disassemble);
