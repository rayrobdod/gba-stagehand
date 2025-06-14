#include <cstdint>
#include <optional>
#include <vector>

/// Four Register Increment Tilemap compression?
/// Rather Acceptable Tilemaps?
///
///
/// All four registers initialized to 0x0000
///
/// XOR
///     || 0 0 | from:2 | to: 2 | hi | low || (if hi) Δhi : 8 || (if low) Δlow ||
/// `to <- from XOR (Δhi << 8 | Δlow)`
///
/// If hi is 0, do not serialize the Δhi byte, and treat as 0 instead
/// If low is 0, do not serialize the Δlow byte, and treat as 0 instead
///
/// RUN
///     || 1 0 | reg : 2 | lengthM1 : 4 || (if lengthM1 == F) lengthM31 : 8 ||
/// length times, write the value in register `reg` to output
///
/// If lengthM1 is not 0b1111, length is lengthM1+1, and lengthM31 is not serialized
/// Otherwise, length is lengthM31 + 31.
///
/// Use two consecutive `RUN`s to represent a RUN between 16 and 30 inclusive
///
/// SLIDE UP
///     || 1 1 | ... (same as RUN)
/// length times, write the value in register `reg` to output and increment the value in `reg`
///
/// SLIDE DOWN
///     || 0 1 | ... (same as RUN)
/// length times, write the value in register `reg` to output and decrement the value in `reg`

std::optional<std::vector<uint8_t>> compressFrit16(std::vector<uint8_t> src);
std::vector<uint8_t> decompressFrit16(std::vector<uint8_t> src, bool decompile);

std::optional<std::vector<uint8_t>> compressFrit8(std::vector<uint8_t> src);
std::vector<uint8_t> decompressFrit8(std::vector<uint8_t> src, bool decompile);
