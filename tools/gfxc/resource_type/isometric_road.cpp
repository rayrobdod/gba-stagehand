#include "resource_type/isometric_road.hpp"

#include <iostream>
#include <stdexcept>
#include "find_palette_superset.hpp"
#include "object.hpp"
#include "struct_bytes_builder.hpp"
#include "resource_type/background.hpp"
#include "resource_type/tileset.hpp"

static void isometric_road_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct isometric_road {" << std::endl
		<< "	struct {" << std::endl
		<< "		bg_tile_t tile[2];" << std::endl
		<< "	} metatile[16];" << std::endl
		<< "};" << std::endl;
}

// assume the source image is properly transparent so that masking isn't required

static std::vector<gbatile_4bpp> isometric_road_extract_tiles(input_path_and_data input, palette_data palettes) {
	std::vector<gbatile_4bpp> retval;
	bufferedimage image = std::get<bufferedimage>(input.second);

	auto range = image.subs(8, 8);
	for (auto subimg : range) {
		uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
		const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

		const gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));
		const gbatile_4bpp_matcher tile1m(tile1);

		if (retval.end() == std::find_if(retval.begin(), retval.end(), tile1m)) {
			retval.push_back(tile1);
		}
	}

	return retval;
}

static void isometric_road_write_to_elf(
	input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const struct isometric_road " << var_name << ";" << std::endl;

	std::vector<bg_tile_t> tilemap = background_extract_map(input, palettes.second, tiles.second);

	StructBytesBuilder serialized;
	for (bg_tile_t entry : tilemap) {
		serialized.push_uint16(entry.to_short());
	}

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized.bytes_arm, serialized.relocs_arm);
	hostelf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized.bytes_x8664, serialized.relocs_x8664);
}

const type_functions isometric_road_type_functions(
	  &isometric_road_write_struct
	, &tileset_extract_palettes
	, &isometric_road_extract_tiles
	, nullptr
	, &isometric_road_write_to_elf
);
