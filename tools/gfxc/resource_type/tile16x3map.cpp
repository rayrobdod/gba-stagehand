#include "tile16x3map.hpp"

#include <iostream>
#include <stdexcept>
#include "background.hpp"
#include "find_palette_superset.hpp"
#include "indexed_insert_only_set.hpp"
#include "object.hpp"

static palette_data_builder tile16x3map_extract_palettes(
		input_path_and_data input) {
	palette_data_builder retval;
	input_tile16x3map mapimage = std::get<input_tile16x3map>(input.second);
	for (input_tile16x3 tile : mapimage.tiles()) {
		for (std::vector<rgba16_t> pixels : tile.pixelss) {
			if (pixels.size()) {
				bufferedimage image(16, 16, pixels, {}, rgb15_t::BLACK, {});
				for (subimage subimg : image.subs(8, 8)) {
					retval.colorss.insert(subimg.palette());
				}
			}
		}
	}
	return retval;
}

static std::vector<gbatile_4bpp> tile16x3map_extract_tiles(
		input_path_and_data input,
		palette_data palettes) {
	static const std::vector<uint8_t> zero_tile_bytes(8*8/2);
	static const gbatile_4bpp zero_tile(zero_tile_bytes);

	input_tile16x3map mapimage = std::get<input_tile16x3map>(input.second);

	std::vector<gbatile_4bpp> retval;
	retval.push_back(zero_tile);

	for (input_tile16x3 tile : mapimage.tiles()) {
		for (std::vector<rgba16_t> pixels : tile.pixelss) {
			if (pixels.size()) {
				bufferedimage image(16, 16, pixels, {}, rgb15_t::BLACK, {});
				for (subimage subimg : image.subs(8, 8)) {
					uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
					const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

					const gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));
					const gbatile_4bpp_matcher tile1m(tile1);

					if (retval.end() == std::find_if(retval.begin(), retval.end(), tile1m)) {
						retval.push_back(tile1);
					}
				}
			}
		}
	}
	return retval;
}

struct tile16x3 {
	uint16_t behavior;
	bg_tile_t tiles[3][4];

	std::vector<uint16_t> to_shorts(void) {
		std::vector<uint16_t> retval = {this->behavior};
		for (unsigned layer = 0; layer < 3; ++layer)
		for (unsigned sub = 0; sub < 4; ++sub) {
			retval.push_back(this->tiles[layer][sub].to_short());
		}
		return retval;
	}

};

bool operator==(const tile16x3& lhs, const tile16x3& rhs) {
	bool retval = lhs.behavior == rhs.behavior;

	for (unsigned layer = 0; layer < 3; ++layer)
	for (unsigned sub = 0; sub < 4; ++sub)
		retval = retval && lhs.tiles[layer][sub] == rhs.tiles[layer][sub];

	return retval;
}

static std::vector<tile16x3> tile16x3map_convert_metatiles(
	input_path_and_data input,
	palette_data palettes,
	tiles_data tileset
) {
	input_tile16x3map mapimage = std::get<input_tile16x3map>(input.second);

	std::vector<tile16x3> retval;
	for (input_tile16x3 input_metatile : mapimage.tiles()) {
		tile16x3 output;
		output.behavior = input_metatile.behavior;

		for (unsigned layer = 0; layer < 3; ++layer) {
			std::vector<rgba16_t> pixels = input_metatile.pixelss[layer];

			if (pixels.size()) {
				std::map<std::string, std::string> text;
				std::map<std::string, std::map<rgba16_t, rgba16_t>> alt_palettes;
				bufferedimage image(16, 16, pixels, text, rgb15_t::BLACK, alt_palettes);
				for (unsigned suby = 0; suby < 2; ++suby)
				for (unsigned subx = 0; subx < 2; ++subx) {
					subimage subimg = image.sub(subx * 8, suby * 8, 8, 8);

					uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
					const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

					const gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));
					gbatile_4bpp tile1h(tile1);
					tile1h.hflip();
					gbatile_4bpp tile1v(tile1);
					tile1v.vflip();
					gbatile_4bpp tile1hv(tile1h);
					tile1hv.vflip();

					unsigned tile_i;
					for (tile_i = 0; tile_i < tileset.tiles.size(); tile_i++) {
						if (tile1 == tileset.tiles[tile_i]) {
							output.tiles[layer][subx + 2 * suby] =
									bg_tile_t(tile_i, false, false, pal_i);
							break;
						}
						if (tile1h == tileset.tiles[tile_i]) {
							output.tiles[layer][subx + 2 * suby] =
									bg_tile_t(tile_i, true, false, pal_i);
							break;
						}
						if (tile1v == tileset.tiles[tile_i]) {
							output.tiles[layer][subx + 2 * suby] =
									bg_tile_t(tile_i, false, true, pal_i);
							break;
						}
						if (tile1hv == tileset.tiles[tile_i]) {
							output.tiles[layer][subx + 2 * suby] =
									bg_tile_t(tile_i, true, true, pal_i);
							break;
						}
					}
					if (tile_i >= tileset.tiles.size()) {
						std::string msg(input.first);
						msg += ": lost tile";
						throw std::logic_error(msg);
					}
				}
			}
		}
		retval.push_back(output);
	}

	return retval;
}

static void tile16x3map_write_to_elf(
	input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles,
	std::string var_name,
	std::ostream& headerstream,
	Object& elf
) {
	headerstream << "extern const struct tile16x3map " << var_name << ";" << std::endl;

	input_tile16x3map mapimage = std::get<input_tile16x3map>(input.second);

	std::string tile16x3set_name("tile16x3set.");
	tile16x3set_name += var_name;

	std::vector<tile16x3> metatiles = tile16x3map_convert_metatiles(input, palettes.second, tiles.second);

	indexed_insert_only_set<tile16x3> metatileset;
	std::vector<uint16_t> metatilemap;
	for (tile16x3 metatile : metatiles) {
		metatilemap.push_back(
			static_cast<uint16_t>(
				metatileset.find_or_push_back(metatile)
			)
		);
	}

	std::vector<uint16_t> serialized_metatileset;
	for (tile16x3 metatile : metatileset) {
		std::vector<uint16_t> serialized_metatile = metatile.to_shorts();
		std::copy(serialized_metatile.begin(), serialized_metatile.end(), std::back_inserter(serialized_metatileset));
	}

	std::vector<uint16_t> serialized = {
		0, 0,
		0, 0,
		static_cast<uint16_t>(tiles.second.tiles.size()),
		0,
		0, 0,
		static_cast<uint16_t>(mapimage.width()),
		static_cast<uint16_t>(mapimage.height()),
	};
	std::copy(metatilemap.begin(), metatilemap.end(), std::back_inserter(serialized));

	std::initializer_list<relocation_template> relocs_metatileset {};
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
		{
			.offset = 12,
			.type = R_ARM_ABS32,
			.symbol_name = tile16x3set_name,
		},
	};

	elf.push_single_variable_rodata_sections({tile16x3set_name, STB_LOCAL}, serialized_metatileset, relocs_metatileset);
	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized, relocs);
}

static void tile16x3map_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "enum WalkaroundBehavior {" << std::endl
		<< "	WB_NORMAL = 0," << std::endl
		<< "	WB_IMPASSABLE = 1," << std::endl
		<< "};" << std::endl
		<< "struct tile16x3 {" << std::endl
		<< "	enum WalkaroundBehavior behavior;" << std::endl
		<< "	bg_tile_t tiles[3][4];" << std::endl
		<< "};" << std::endl
		<< "_Static_assert(26 == sizeof(struct tile16x3));" << std::endl
		<< std::endl
		<< "struct tile16x3map {" << std::endl
		<< "	struct tileset tileset;" << std::endl
		<< "	struct tile16x3* metatileset;" << std::endl
		<< "	uint16_t width;" << std::endl
		<< "	uint16_t height;" << std::endl
		<< "	uint16_t metatilemap[];" << std::endl
		<< "};" << std::endl;
}

const type_functions tile16x3map_type_functions(
	  &tile16x3map_write_struct
	, &tile16x3map_extract_palettes
	, &tile16x3map_extract_tiles
	, &tile16x3map_write_to_elf
);
