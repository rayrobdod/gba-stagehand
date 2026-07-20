#include "resource_type/background_horizontal_scroll.hpp"

#include "find_palette_superset.hpp"
#include "object.hpp"
#include "resource_type/background.hpp"
#include "resource_type/tileset.hpp"

static void background_horizontal_scroll_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct background_horizontal_scroll {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const struct CompressedData* tileset;" << std::endl
		<< "	const bg_tile_t* tilemap;" << std::endl
		<< "	const uint16_t palette_count;" << std::endl
		<< "	const uint16_t tileset_count;" << std::endl
		<< "	const uint16_t tilemap_height;" << std::endl
		<< "	const uint16_t tilemap_width;" << std::endl
		<< "};" << std::endl;
}

static void background_horizontal_scroll_write_to_elf(
	input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles_pair,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	[[maybe_unused]] Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const struct background_horizontal_scroll " << var_name << ";" << std::endl;

	bufferedimage image = std::get<bufferedimage>(input.second);

	uint16_t map_height(image.height() / 8);
	uint16_t map_width(image.width() / 8);

	std::vector<bg_tile_t> tilemap = background_extract_map(input, palettes.second, tiles_pair.second);
	std::vector<bg_tile_t> tilemap_transpose(tilemap.size());
	for (unsigned y = 0; y < map_height; y++) {
		for (unsigned x = 0; x < map_width; x++) {
			tilemap_transpose[y + x * map_height] = tilemap[x + y * map_width];
		}
	}

	std::array<uint16_t, 10> serialized = {
		0, 0,
		0, 0,
		0, 0,
		static_cast<uint16_t>(palettes.second.colorss.size()),
		static_cast<uint16_t>(tiles_pair.second.tiles.size()),
		map_height,
		map_width,
	};

	std::string pal_name = palettes.first;
	std::string tileset_name = tiles_pair.first;
	std::string tilemap_name("map.");
	tilemap_name += var_name;

	std::initializer_list<relocation_template> relocs {
		{
			.offset = 0,
			.type = R_ARM_ABS32,
			.symbol_name = pal_name,
		},
		{
			.offset = 4,
			.type = R_ARM_ABS32,
			.symbol_name = tileset_name,
		},
		{
			.offset = 8,
			.type = R_ARM_ABS32,
			.symbol_name = tilemap_name,
		},
	};

	std::vector<uint8_t> tilemap_bytes;
	for (bg_tile_t entry : tilemap_transpose) {
		std::array<uint8_t, 2> bs = entry.to_bytes();
		tilemap_bytes.push_back(bs[0]);
		tilemap_bytes.push_back(bs[1]);
	}

	elf.push_single_variable_rodata_sections({tilemap_name, STB_LOCAL}, tilemap_bytes);
	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized, relocs);
}

const type_functions background_horizontal_scroll_type_functions(
	  &background_horizontal_scroll_write_struct
	, &tileset_extract_palettes
	, &background_extract_tiles
	, nullptr
	, &background_horizontal_scroll_write_to_elf
);
