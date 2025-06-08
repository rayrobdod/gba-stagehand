#include "choose_compression.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include "compression/frit.hpp"
#include "compression/huff.hpp"
#include "compression/lz.hpp"
#include "compression/rl.hpp"

struct choosable_compression {
	std::string_view alg_name;
	std::vector<uint8_t> (*compress)(std::vector<uint8_t> src);
	std::vector<uint8_t> (*decompress)(std::vector<uint8_t> src, bool decompile);
};

using namespace std::string_view_literals;
const static std::array<choosable_compression, 7> compression_algs = {{
	{"LZ"sv,	&compressLz,	&decompressLz,},
	{"LZ11"sv,	&compressLz11,	&decompressLz11,},
	{"RL"sv,	&compressRl,	&decompressRl,},
	{"FRIT16"sv,	&compressFrit16,	&decompressFrit16,},
	{"FRIT8"sv,	&compressFrit8,	&decompressFrit8,},
	{"Huff8"sv,	&compressHuff8,	&decompressHuff8,},
	{"Huff4"sv,	&compressHuff4,	&decompressHuff4,},
}};

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
		std::vector<uint8_t> compressed = alg.compress(data);
		std::vector<uint8_t> round = alg.decompress(compressed, false);

		if (data != round) {
			std::string msg;
			msg += "Compressing ";
			msg += tiles_name;
			msg += " using ";
			msg += alg.alg_name;
			msg += " failed to round trip\n";
			auto diff = std::mismatch(data.begin(), data.end(), round.begin(), round.end());
			std::vector<unsigned char>::size_type at = diff.first - data.begin();
			std::vector<unsigned char>::size_type range_start = (at > 3 ? at - 3 : 0);
			msg += "    at: " + std::to_string(at) + "\n";
			msg += "    data : {";
			msg += (range_start != 0 ? "... " : "");
			for (unsigned i = range_start; i < std::min(range_start + 5, data.size()); i++) {
				char s[8];
				sprintf(s, "%02X, ", data[range_start + i]);
				msg += s;
			}
			msg += "}\n";
			msg += "    round: {";
			msg += (range_start != 0 ? "... " : "");
			for (unsigned i = range_start; i < std::min(range_start + 5, round.size()); i++) {
				char s[8];
				sprintf(s, "%02X, ", round[range_start + i]);
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
		if (compressed.size() < retval.data.size()) {
			retval.alg_name = alg.alg_name;
			retval.data = compressed;
		}
	}

	std::cout << tiles_name << " " << retval.alg_name << " " << retval.data.size() << std::endl;

	return retval;
}
