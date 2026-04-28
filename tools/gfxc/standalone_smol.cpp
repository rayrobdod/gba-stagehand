#include <algorithm>
#include <iterator>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include "compression/smol.hpp"
#include "compare_mismatch_error_message.hpp"

struct Arguments {
	enum {
		NORMAL,
		MODE,
		POSITIONAL_ONLY,
	} state;
	enum {
		COMPRESS = 0,
		INFLATE,
	} action;
	const char* programName;
	std::filesystem::path inName;
	std::filesystem::path outName;
	int mode;
	bool disassemble;

	Arguments() :
		state(NORMAL)
		, action(COMPRESS)
		, programName(NULL)
		, mode(-1)
		{}
};

struct compression_variants {
	std::optional<std::vector<uint8_t>> (*compress)(std::vector<uint8_t> src);
	std::vector<uint8_t> (*decompress)(std::vector<uint8_t> src, bool disassemble);
};

static const std::initializer_list<compression_variants> compression_algs = {
	{NULL, NULL},
	{&compressSmol1,	&decompressSmol1,},
	{&compressSmol2,	&decompressSmol2,},
	{&compressSmol3,	&decompressSmol3,},
	{&compressSmol4,	&decompressSmol4,},
	{&compressSmol5,	&decompressSmol5,},
	{&compressSmol6,	&decompressSmol6,},
	{NULL, NULL},
	{&compressSmol8,	&decompressSmol8,},
};

int main(int argc, char* argv[]) {
	struct Arguments args;
	for (int i = 0; i < argc; i++) {
		if (Arguments::MODE == args.state) {
			if ('0' < argv[i][0] && argv[i][0] <= '6' && argv[i][1] == '\0')
				args.mode = argv[i][0] - '0';
			else if ('8' == argv[i][0] && argv[i][1] == '\0')
				args.mode = 8;
			else if (0 == strcmp("tilemap", argv[i]))
				args.mode = 8;
			else {
				fprintf(stderr, "Error: Unknown mode\n");
				return 1;
			}
			args.state = Arguments::NORMAL;
		}
		else if ('-' != argv[i][0] || Arguments::POSITIONAL_ONLY == args.state) {
			if (!args.programName)
				args.programName = argv[i];
			else if (args.inName.empty())
				args.inName = argv[i];
			else if (args.outName.empty())
				args.outName = argv[i];
			else {
				fprintf(stderr, "Error: Too many positional args\n");
				return 1;
			}
		}
		else if (0 == strcmp("-d", argv[i])) {
			args.action = Arguments::INFLATE;
		}
		else if (0 == strcmp("-m", argv[i])) {
			args.state = Arguments::MODE;
		}
		else if (0 == strcmp("-v", argv[i])) {
			args.disassemble = true;
		}
		else if (0 == strncmp("--", argv[i], 2)) {
			args.state = Arguments::POSITIONAL_ONLY;
		}
		else {
			fprintf(stderr, "Error: Unknown argument\n");
			return 1;
		}
	}

	std::vector<uint8_t> inData;
	{
		using namespace std::literals::string_literals;
		inData.resize(std::filesystem::file_size(args.inName));
		std::ifstream inStream;
		inStream.open(args.inName, std::ios::binary);
		if (!inStream.is_open()) {
			throw std::runtime_error("Couldn't open "s + args.inName.string() + " for reading bytes\n");
		}
		inStream.read(reinterpret_cast<char*>(inData.data()), inData.size());
		if (static_cast<unsigned long int>(inStream.gcount()) != inData.size()) {
			throw std::runtime_error("Couldn't fully read "s + args.inName.string());
		}
		inStream.close();
	}

	std::vector<uint8_t> outData;

	switch (args.action) {
	case Arguments::INFLATE: {
		unsigned variant = inData[0] & 0xF;
		if (variant > compression_algs.size()) {
			fprintf(stderr, "Error: Unsupported variant: %x\n", variant);
			return 2;
		}
		std::vector<uint8_t> (*decompressFn)(std::vector<uint8_t> src, bool disassemble) = (std::data(compression_algs) + variant)->decompress;
		if (NULL == decompressFn) {
			fprintf(stderr, "Error: Unsupported variant: %x\n", variant);
			return 2;
		}

		inData.insert(inData.begin(), 4, 0);

		outData = decompressFn(inData, args.disassemble);
	} break;
	case Arguments::COMPRESS: {
		if (args.mode < 0) {
			for (auto alg : compression_algs) {
				if (! alg.compress)
					continue;

				std::optional<std::vector<uint8_t>> compressedOpt = alg.compress(inData);
				if (! compressedOpt)
					continue;

				std::vector<uint8_t> compressed = *compressedOpt;
				std::vector<uint8_t> round = alg.decompress(compressed, args.disassemble);

				if (inData != round) {
					throw std::logic_error(compareMismatchErrorMessage(inData, compressed, round, "", ""));
				}
				if (inData == round && (outData.size() == 0 || compressed.size() < outData.size())) {
					outData = compressed;
				}
			}
			if (outData.size() < 4) {
				throw std::logic_error("Failed to compress");
			}
			outData.erase(outData.begin(), outData.begin() + 4);
		} else {
			auto alg = (std::data(compression_algs) + args.mode);

			std::optional<std::vector<uint8_t>> compressedOpt = alg->compress(inData);
			if (! compressedOpt)
				throw std::logic_error("Failed to compress");

			outData = *compressedOpt;
			std::vector<uint8_t> round = alg->decompress(outData, false);

			if (inData != round) {
				throw std::logic_error(compareMismatchErrorMessage(inData, outData, round, "", ""));
			}

			outData.erase(outData.begin(), outData.begin() + 4);
		}
	} break;
	}

	std::ofstream outStream(args.outName, std::ios::out | std::ios::binary);
	outStream.write(reinterpret_cast<const char *>(outData.data()), outData.size());
	outStream.close();
}
