#ifndef RESOURCE_TYPE_FUNCTIONS_HPP
#define RESOURCE_TYPE_FUNCTIONS_HPP

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include "input_data_variant.hpp"
#include "resource_type.hpp"

struct palette_data_builder;

struct alt_palette_data {
	uint16_t tag;
	std::map<rgba16_t, rgba16_t> mapping;

	alt_palette_data(uint16_t tag, std::map<rgba16_t, rgba16_t>);
};

struct palette_data {
	uint16_t tag;
	std::vector<std::vector<rgba16_t>> colorss;
	std::map<const std::string, alt_palette_data> alternates;

	palette_data();
	palette_data(
		  uint16_t tag
		, std::vector<std::vector<rgba16_t>> colorss
		, std::map<const std::string, alt_palette_data> alternates
	);
	palette_data(
		  uint16_t tag
		, palette_data_builder builder
	);
};

struct palette_data_builder {
	std::set<std::set<rgba16_t>> colorss;
	std::map<std::string, std::map<rgba16_t, rgba16_t>> alternates;

	palette_data_builder();

	void merge(palette_data_builder&, std::string_view palette_name);
	void condense_colors();
};

struct tiles_data {
	uint16_t tag;
	std::vector<gbatile_4bpp> tiles;

	tiles_data();
	tiles_data(uint16_t tag, std::vector<gbatile_4bpp>);
};

struct bg_tile_t {
	uint16_t tile : 10;
	bool hflip : 1;
	bool vflip : 1;
	uint16_t palette : 4;

	bg_tile_t();
	bg_tile_t(uint16_t tile, bool hflip, bool vflip, uint16_t palette);
	std::array<uint8_t, 2> to_bytes(void) const;
	uint16_t to_short(void) const;
};
bool operator==(const bg_tile_t& lhs, const bg_tile_t& rhs);

struct tile16x3 {
	uint16_t behavior;
	bg_tile_t tiles[3][4];

	std::vector<uint16_t> to_shorts(void) const;
};
bool operator==(const tile16x3& lhs, const tile16x3& rhs);

struct tile16x3s_data {
	std::vector<tile16x3> tile16x3s;
	tile16x3s_data();
	tile16x3s_data(std::vector<tile16x3>);
};

class Object;

struct type_functions {
	void (*write_struct)(std::ostream&);
	palette_data_builder (*extract_palettes)(input_path_and_data);
	std::vector<gbatile_4bpp> (*extract_tiles)(input_path_and_data, palette_data);
	std::vector<tile16x3> (*extract_tile16x3s)(
		input_path_and_data input,
		palette_data palettes,
		tiles_data tileset
	);
	void (*write_to_elf)(
		input_path_and_data,
		std::pair<std::string, palette_data> palettes,
		std::pair<std::string, tiles_data> tiles,
		std::pair<std::string, tile16x3s_data> tile16x3s,
		std::string var_name,
		std::ostream& headerstream,
		Object& elf
	);
};

extern const std::map<type, type_functions> type_functionss;

#endif
