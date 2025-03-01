#include <cstdint>
#include <vector>

std::vector<uint8_t> compressLz(std::vector<uint8_t> src);
std::vector<uint8_t> decompressLz(std::vector<uint8_t> src, bool decompile);
