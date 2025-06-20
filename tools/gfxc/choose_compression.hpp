#ifndef CHOOSE_COMPRESSION_HPP
#define CHOOSE_COMPRESSION_HPP

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

struct choosable_compression {
	std::string_view alg_name;
	std::optional<std::vector<uint8_t>> (*compress)(std::vector<uint8_t> src);
	std::vector<uint8_t> (*decompress)(std::vector<uint8_t> src, bool disassemble);
};

extern const std::initializer_list<choosable_compression> compression_algs;

struct choosen_compression {
	std::string_view alg_name;
	std::vector<uint8_t> data;
};

choosen_compression choose_compression(std::string tiles_name, std::vector<uint8_t> data);

#endif        //  #ifndef CHOOSE_COMPRESSION_HPP
