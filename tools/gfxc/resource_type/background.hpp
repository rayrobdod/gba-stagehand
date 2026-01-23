#ifndef RESOURCE_TYPE_BACKGROUND_HPP
#define RESOURCE_TYPE_BACKGROUND_HPP

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>
#include "image.hpp"
#include "resource_type_functions.hpp"

std::vector<gbatile_4bpp> background_extract_tiles(input_path_and_data, palette_data);
std::vector<bg_tile_t> background_extract_map(input_path_and_data, palette_data, tiles_data);

extern const type_functions background_type_functions;

#endif
