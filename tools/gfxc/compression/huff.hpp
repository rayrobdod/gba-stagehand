#include <cstdint>
#include <vector>

std::vector<uint8_t> compressHuff4(std::vector<uint8_t> src);
std::vector<uint8_t> decompressHuff4(std::vector<uint8_t> src, bool decompile);

std::vector<uint8_t> compressHuff8(std::vector<uint8_t> src);
std::vector<uint8_t> decompressHuff8(std::vector<uint8_t> src, bool decompile);
