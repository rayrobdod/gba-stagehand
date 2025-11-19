#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

std::string compareMismatchErrorMessage(
		std::vector<uint8_t> original,
		std::vector<uint8_t> compressed,
		std::vector<uint8_t> round,
		std::string_view alg_name,
		std::string_view variable_name);
