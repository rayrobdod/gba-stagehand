#ifndef RESOURCE_TYPE_BACKGROUND_HPP
#define RESOURCE_TYPE_BACKGROUND_HPP

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "image.hpp"
#include "object.hpp"
#include "resource_type_functions.hpp"

struct bg_tile_t {
	uint16_t tile : 10;
	bool hflip : 1;
	bool vflip : 1;
	uint16_t palette : 4;

	bg_tile_t(uint16_t tile, bool hflip, bool vflip, uint16_t palette);
	std::array<uint8_t, 2> to_bytes(void);
};

struct background {
	std::string var_name;
	std::vector<std::array<rgba16_t, 16>> palette;
	std::vector<uint8_t> tileset;
	std::vector<bg_tile_t> tilemap;

	explicit background(const std::pair<std::filesystem::path, bufferedimage>);
	void write(std::ostream& headerstream, Object& elf) const;
};

extern const type_functions background_type_functions;

#endif
