#include "choose_compression.hpp"

#include <array>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include "compression/frit.hpp"
#include "compression/huff.hpp"
#include "compression/lz.hpp"
#include "compression/rl.hpp"
#include "compression/rlzero.hpp"

using namespace std::string_view_literals;
const std::initializer_list<choosable_compression> compression_algs = {
	{"LZ"sv,	&compressLz,	&decompressLz,},
	{"LZ11"sv,	&compressLz11,	&decompressLz11,},
	{"RL"sv,	&compressRl,	&decompressRl,},
	{"RLZero"sv,	&compressRlZero,	&decompressRlZero,},
	{"FRIT16"sv,	&compressFrit16,	&decompressFrit16,},
	{"FRIT8"sv,	&compressFrit8,	&decompressFrit8,},
	{"Huff8"sv,	&compressHuff8,	&decompressHuff8,},
	{"Huff4"sv,	&compressHuff4,	&decompressHuff4,},
};

choosen_compression choose_compression(std::string tiles_name, std::vector<uint8_t> data) {
	choosen_compression retval;
	retval.alg_name = "IDENT"sv;
	retval.data.clear();
	retval.data.push_back(0);
	retval.data.push_back(data.size());
	retval.data.push_back(data.size() >> 8);
	retval.data.push_back(data.size() >> 16);
	std::copy(data.begin(), data.end(), std::back_inserter(retval.data));

	for (auto alg : compression_algs) {
		std::optional<std::vector<uint8_t>> compressedOpt = alg.compress(data);
		if (! compressedOpt)
			continue;

		std::vector<uint8_t> compressed = *compressedOpt;
		std::vector<uint8_t> round = alg.decompress(compressed, false && tiles_name == "tile.1002" && alg.alg_name == "FRIT8"sv);

		if (data != round) {
			std::string msg;
			msg += "Compressing ";
			msg += tiles_name;
			msg += " using ";
			msg += alg.alg_name;
			msg += " failed to round trip\n";
			auto diff = std::mismatch(data.begin(), data.end(), round.begin(), round.end());
			std::vector<unsigned char>::size_type at = diff.first - data.begin();
			std::vector<unsigned char>::size_type range_start = (at > 2 ? at - 2 : 0);
			msg += "    at: " + std::to_string(at) + "\n";
			msg += "    data : {";
			msg += (range_start != 0 ? "... " : "");
			for (unsigned i = range_start; i < std::min(range_start + 5, data.size()); i++) {
				char s[8];
				sprintf(s, "%02X, ", data[i]);
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
			for (unsigned i = 0; i < data.size(); i++) {
				char s[8];
				sprintf(s, "%02X, ", compressed[i]);
				msg += s;
			}
			msg += "}\n";

			throw std::logic_error(msg);
		}
		if (data == round && compressed.size() < retval.data.size()) {
			retval.alg_name = alg.alg_name;
			retval.data = compressed;
		}
	}

	if (false)
		std::cout << "    " << std::left << std::setw(35) << tiles_name
			<< ": " << std::setw(8) << retval.alg_name
			<< ": " << std::right << std::setw(6) << retval.data.size() << std::endl;

	return retval;
}
