#ifndef RESOURCE_TYPE_FUNCTIONS_HPP
#define RESOURCE_TYPE_FUNCTIONS_HPP

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include "image.hpp"
#include "resource_type.hpp"

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

class Object;

struct type_functions {
	void (*write_struct)(std::ostream&);
	palette_data_builder (*extract_palettes)(std::pair<std::filesystem::path, struct bufferedimage>);
	std::vector<gbatile_4bpp> (*extract_tiles)(std::pair<std::filesystem::path, struct bufferedimage>, palette_data);
	void (*write_to_elf)(
		std::pair<std::filesystem::path, struct bufferedimage> image,
		std::pair<std::string, palette_data> palettes,
		std::pair<std::string, tiles_data> tiles,
		std::string var_name,
		std::ostream& headerstream,
		Object& elf
	);
};

extern const std::map<type, type_functions> type_functionss;

#endif
