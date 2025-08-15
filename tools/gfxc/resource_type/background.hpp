#ifndef RESOURCE_TYPE_BACKGROUND_HPP
#define RESOURCE_TYPE_BACKGROUND_HPP

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>
#include "image.hpp"
#include "resource_type_functions.hpp"

struct bg_tile_t {
	uint16_t tile : 10;
	bool hflip : 1;
	bool vflip : 1;
	uint16_t palette : 4;

	bg_tile_t();
	bg_tile_t(uint16_t tile, bool hflip, bool vflip, uint16_t palette);
	std::array<uint8_t, 2> to_bytes(void);
};

std::vector<gbatile_4bpp> background_extract_tiles(std::pair<std::filesystem::path, struct bufferedimage> image, palette_data palettes);
std::vector<bg_tile_t> background_extract_map(std::pair<std::filesystem::path, struct bufferedimage> image, palette_data palettes, tiles_data tiles_dat);

extern const type_functions background_type_functions;

#endif
