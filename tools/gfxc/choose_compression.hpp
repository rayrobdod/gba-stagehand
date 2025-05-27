#ifndef CHOOSE_COMPRESSION_HPP
#define CHOOSE_COMPRESSION_HPP

#include <cstdint>
#include <string_view>
#include <vector>

struct choosen_compression {
	std::string_view alg_name;
	std::vector<uint8_t> data;
};

choosen_compression choose_compression(std::string tiles_name, std::vector<uint8_t> data);

#endif        //  #ifndef CHOOSE_COMPRESSION_HPP
