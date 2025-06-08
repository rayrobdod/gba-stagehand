#include <cstdint>
#include <optional>
#include <vector>

std::optional<std::vector<uint8_t>> compressLz(std::vector<uint8_t> src);
std::vector<uint8_t> decompressLz(std::vector<uint8_t> src, bool decompile);

std::optional<std::vector<uint8_t>> compressLz11(std::vector<uint8_t> src);
std::vector<uint8_t> decompressLz11(std::vector<uint8_t> src, bool decompile);
