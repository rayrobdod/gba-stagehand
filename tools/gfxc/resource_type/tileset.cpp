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
		<< "	uint16_t palette_count;" << std::endl
		<< "	paltag_t paltag;" << std::endl
		<< "	const struct CompressedData* tileset;" << std::endl
		<< "	uint16_t tileset_count;" << std::endl
		<< "	tiletag_t tiletag;" << std::endl
		<< "};" << std::endl;
}

void tileset_serialized(
		std::span<uint16_t, 8> bytebuffer,
		std::span<relocation_template, 2> relocs,
		std::span<uint16_t, 16> bytebuffer_x8664,
		std::span<relocation_template_x8664, 2> relocs_x8664,
		const std::pair<std::string, palette_data> palettes,
		const std::pair<std::string, tiles_data> tiles) {

	bytebuffer[0] = 0;
	bytebuffer[1] = 0;
	bytebuffer[2] = static_cast<uint16_t>(palettes.second.colorss.size());
	bytebuffer[3] = palettes.second.tag;
	bytebuffer[4] = 0;
	bytebuffer[5] = 0;
	bytebuffer[6] = static_cast<uint16_t>(tiles.second.tiles.size());
	bytebuffer[7] = tiles.second.tag;

	relocs[0] = {
		.offset = 0,
		.type = R_ARM_ABS32,
		.symbol_name = palettes.first,
	};
	relocs[1] = {
		.offset = 8,
		.type = R_ARM_ABS32,
		.symbol_name = tiles.first,
	};


	bytebuffer_x8664[0] = 0;
	bytebuffer_x8664[1] = 0;
	bytebuffer_x8664[2] = 0;
	bytebuffer_x8664[3] = 0;
	bytebuffer_x8664[4] = static_cast<uint16_t>(palettes.second.colorss.size());
	bytebuffer_x8664[5] = palettes.second.tag;
	bytebuffer_x8664[6] = 0;
	bytebuffer_x8664[7] = 0;
	bytebuffer_x8664[8] = 0;
	bytebuffer_x8664[9] = 0;
	bytebuffer_x8664[10] = 0;
	bytebuffer_x8664[11] = 0;
	bytebuffer_x8664[12] = static_cast<uint16_t>(tiles.second.tiles.size());
	bytebuffer_x8664[13] = tiles.second.tag;
	bytebuffer_x8664[14] = 0;
	bytebuffer_x8664[15] = 0;

	relocs_x8664[0] = {
		.offset = 0,
		.type = R_X86_64_64,
		.symbol_name = palettes.first,
	};
	relocs_x8664[1] = {
		.offset = 16,
		.type = R_X86_64_64,
		.symbol_name = tiles.first,
	};
}

static void tileset_write_to_elf(
	[[gnu::unused]] input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	[[maybe_unused]] Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const struct tileset " << var_name << ";" << std::endl;

	std::array<uint16_t, 8> serialized = {0};
	std::array<relocation_template, 2> relocs = {0};
	std::array<uint16_t, 16> serialized_x8664 = {0};
	std::array<relocation_template_x8664, 2> relocs_x8664 = {0};

	tileset_serialized(serialized, relocs, serialized_x8664, relocs_x8664, palettes, tiles);

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized, relocs);
	hostelf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized_x8664, relocs_x8664);
}

const type_functions tileset_type_functions(
	  &tileset_write_struct
	, &tileset_extract_palettes
	, &tileset_extract_tiles
	, nullptr
	, &tileset_write_to_elf
);
