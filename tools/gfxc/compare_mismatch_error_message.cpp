#include "compare_mismatch_error_message.hpp"

std::string compareMismatchErrorMessage(
		std::vector<uint8_t> original,
		std::vector<uint8_t> compressed,
		std::vector<uint8_t> round,
		std::string_view alg_name,
		std::string_view variable_name) {
	std::string msg;
	msg += "Compressing ";
	msg += variable_name;
	msg += " using ";
	msg += alg_name;
	msg += " failed to round trip\n";
	auto diff = std::mismatch(original.begin(), original.end(), round.begin(), round.end());
	std::vector<unsigned char>::size_type at = diff.first - original.begin();
	std::vector<unsigned char>::size_type range_start = (at > 2 ? at - 2 : 0);
	msg += "    at: " + std::to_string(at) + "\n";
	msg += "    original : {";
	msg += (range_start != 0 ? "... " : "");
	for (unsigned i = range_start; i < std::min(range_start + 5, original.size()); i++) {
		char s[8];
		sprintf(s, "%02X, ", original[i]);
		msg += s;
	}
	msg += "}\n";
	msg += "    round: {";
	msg += (range_start != 0 ? "... " : "");
	for (unsigned i = range_start; i < std::min(range_start + 5, round.size()); i++) {
		char s[8];
		sprintf(s, "%02X, ", round[i]);
		msg += s;
	}
	msg += "}\n";
	msg += "    compressed: {";
	for (unsigned i = 0; i < original.size(); i++) {
		char s[8];
		sprintf(s, "%02X, ", compressed[i]);
		msg += s;
	}
	msg += "}\n";

	return msg;
}