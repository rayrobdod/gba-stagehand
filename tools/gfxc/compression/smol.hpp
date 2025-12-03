#include <cstdint>
#include <optional>
#include <vector>

std::optional<std::vector<uint8_t>> compressSmol1(std::vector<uint8_t> src);
std::optional<std::vector<uint8_t>> compressSmol2(std::vector<uint8_t> src);
std::optional<std::vector<uint8_t>> compressSmol4(std::vector<uint8_t> src);
std::optional<std::vector<uint8_t>> compressSmol5(std::vector<uint8_t> src);
std::vector<uint8_t> decompressSmol1(std::vector<uint8_t> src, bool disassemble);
std::vector<uint8_t> decompressSmol2(std::vector<uint8_t> src, bool disassemble);
std::vector<uint8_t> decompressSmol4(std::vector<uint8_t> src, bool disassemble);
std::vector<uint8_t> decompressSmol5(std::vector<uint8_t> src, bool disassemble);
