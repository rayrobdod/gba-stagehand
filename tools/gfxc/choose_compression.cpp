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
#include "compression/smol.hpp"
#include "compare_mismatch_error_message.hpp"

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
	{"Smol1"sv,	&compressSmol1,	&decompressSmol,},
	{"Smol2"sv,	&compressSmol2,	&decompressSmol,},
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
		std::vector<uint8_t> round = alg.decompress(compressed, false && alg.alg_name == "Smol2"sv);

		if (data != round) {
			throw std::logic_error(compareMismatchErrorMessage(data, compressed, round, alg.alg_name, tiles_name));
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
