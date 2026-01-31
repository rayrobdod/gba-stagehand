#include "tileset.hpp"

#include <iostream>
#include <stdexcept>
#include "find_palette_superset.hpp"
#include "object.hpp"

palette_data_builder tileset_extract_palettes(input_path_and_data input) {
	palette_data_builder retval;

	bufferedimage image = std::get<bufferedimage>(input.second);
	rgba16_t background = image.background().with_alpha(0);
	std::set<std::set<rgba16_t>> tile_palettes;
	for (auto subimg : image.subs(8, 8)) {
		std::set<rgba16_t> new_pal;
		new_pal.insert(background);
		new_pal.merge(subimg.palette());
		if (new_pal.size() > 16) {
			std::string msg(input.first.string());
			msg += ": tile palette larger than 16 colors";
			throw std::logic_error(msg);
		}
		tile_palettes.insert(new_pal);
	}

	for (auto pal : tile_palettes) {
		retval.colorss.insert(pal);
	}

	return retval;
}

static std::vector<gbatile_4bpp> tileset_extract_tiles(input_path_and_data input, palette_data palettes) {
	std::vector<gbatile_4bpp> retval;
	bufferedimage image = std::get<bufferedimage>(input.second);

	for (auto subimg : image.subs(8, 8)) {
		uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
		const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

		gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

		retval.push_back(tile1);
	}

	return retval;
}

static void tileset_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct tileset {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const struct CompressedData* tileset;" << std::endl
		<< "	const uint16_t tileset_count;" << std::endl
		<< "};" << std::endl;
}

static void tileset_write_to_elf(
	[[gnu::unused]] input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	Object& elf
) {
	headerstream << "extern const struct tileset " << var_name << ";" << std::endl;

	std::array<uint16_t, 6> serialized = {
		0, 0,
		0, 0,
		static_cast<uint16_t>(tiles.second.tiles.size()),
		0,
	};

	std::initializer_list<relocation_template> relocs {
		{
			.offset = 0,
			.type = R_ARM_ABS32,
			.symbol_name = palettes.first,
		},
		{
			.offset = 4,
			.type = R_ARM_ABS32,
			.symbol_name = tiles.first,
		},
	};

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized, relocs);
}

const type_functions tileset_type_functions(
	  &tileset_write_struct
	, &tileset_extract_palettes
	, &tileset_extract_tiles
	, nullptr
	, &tileset_write_to_elf
);
