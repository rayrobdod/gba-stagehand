#include "tile16x3map.hpp"

#include <iostream>
#include <numeric>
#include <stdexcept>
#include <unordered_set>
#include "find_palette_superset.hpp"
#include "indexed_insert_only_set.hpp"
#include "object.hpp"
#include "struct_bytes_builder.hpp"
#include "tileset.hpp"

template<>
struct std::hash<tile16x3> {
	std::size_t operator()(const tile16x3& value) const noexcept {
		std::vector<uint16_t> ser = value.to_shorts();
		return std::accumulate(ser.begin(), ser.end(), 1,
			[](std::size_t folding, uint16_t a) {return folding * 7 + a;});
	}
};

static palette_data_builder tile16x3map_extract_palettes(
		input_path_and_data input) {
	palette_data_builder retval;
	input_tile16x3map mapimage = std::get<input_tile16x3map>(input.second);
	for (input_tile16x3 tile : mapimage.tiles()) {
		for (std::vector<rgba16_t> pixels : tile.pixelss) {
			if (pixels.size()) {
				bufferedimage image(16, 16, pixels, {}, rgb15_t::BLACK, {}, {});
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
				bufferedimage image(16, 16, pixels, {}, rgb15_t::BLACK, {}, {});
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

static std::vector<tile16x3> tile16x3map_convert_to_metatiles(
	input_path_and_data input,
	palette_data palettes,
	tiles_data tileset
) {
	input_tile16x3map mapimage = std::get<input_tile16x3map>(input.second);

	std::vector<tile16x3> retval;
	for (input_tile16x3 input_metatile : mapimage.tiles()) {
		tile16x3 output;
		output.behavior = input_metatile.behavior_id;

		for (unsigned layer = 0; layer < 3; ++layer) {
			std::vector<rgba16_t> pixels = input_metatile.pixelss[layer];

			if (pixels.size()) {
				std::map<std::string, std::string> text;
				std::map<std::string, std::map<rgba16_t, rgba16_t>> alt_palettes;
				bufferedimage image(16, 16, pixels, text, rgb15_t::BLACK, alt_palettes, {});
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

static std::vector<tile16x3> tile16x3map_extract_metatiles(
	input_path_and_data input,
	palette_data palettes,
	tiles_data tileset
) {
	// order is irrelevant. Access pattern prevents compression, beyond metatiling, so order doesn't even matter for that.
	std::vector<tile16x3> _a = tile16x3map_convert_to_metatiles(input, palettes, tileset);
	std::unordered_set<tile16x3> _b(_a.begin(), _a.end());
	std::vector<tile16x3> _c(_b.begin(), _b.end());
	return _c;
}

static void tile16x3map_write_to_elf(
	input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles,
	std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const struct tile16x3map " << var_name << ";" << std::endl;

	input_tile16x3map mapimage = std::get<input_tile16x3map>(input.second);

	std::vector<tile16x3> metatiles = tile16x3map_convert_to_metatiles(input, palettes.second, tiles.second);

	std::vector<tile16x3> metatileset = tile16x3s.second.tile16x3s;

	std::vector<uint16_t> metatilemap;
	for (tile16x3 metatile : metatiles) {
		auto result = std::find(metatileset.begin(), metatileset.end(), metatile);
		if (metatileset.end() == result) {
			std::string msg(input.first);
			msg += ": lost tile";
			throw std::logic_error(msg);
		}

		metatilemap.push_back(
			static_cast<uint16_t>(result - metatileset.begin())
		);
	}

	std::string strings_name("stringdata.");
	strings_name += var_name;
	StringTableBuilder strings;

	std::string signs_name("signsdata.");
	signs_name += var_name;
	StructBytesBuilder signs;
	for (auto sign : mapimage.signs()) {
		signs.push_uint16(static_cast<uint16_t>(sign.x));
		signs.push_uint16(static_cast<uint16_t>(sign.y));
		signs.push_pointer(strings_name, strings.find_or_push(sign.message));
		signs.self_align();
	}

	std::string warps_name("warpsdata.");
	warps_name += var_name;
	uint16_t warp_index = 0;
	StructBytesBuilder warps;
	for (auto warp : mapimage.warps()) {
		std::string warp_symbol_name = var_name + "." + warp.name;
		elf.push_symbol({
			.name = warp_symbol_name,
			.st_value = warp_index,
			.st_size = 0,
			.binding = STB_LOCAL,
			.type = STT_NOTYPE,
			.st_other = STV_DEFAULT,
			.section = "<ABS>",
		});
		hostelf.push_symbol({
			.name = warp_symbol_name,
			.st_value = warp_index,
			.st_size = 0,
			.binding = STB_LOCAL,
			.type = STT_NOTYPE,
			.st_other = STV_DEFAULT,
			.section = "<ABS>",
		});
		warp_index += 1;

		warps.push_uint16(static_cast<uint16_t>(warp.x));
		warps.push_uint16(static_cast<uint16_t>(warp.y));
		warps.push_pointer(warp.destination_map);
		warps.push_uint16_relocated(warp.destination_map + "." + warp.destination_warp);
		warps.self_align();
	}

	StructBytesBuilder serialized;
	serialized.push_tileset(palettes, tiles);
	serialized.push_pointer(tile16x3s.first);
	serialized.push_pointer(signs_name);
	serialized.push_pointer(warps_name);
	serialized.push_uint16(static_cast<uint16_t>(mapimage.signs().size()));
	serialized.push_uint16(static_cast<uint16_t>(mapimage.warps().size()));
	serialized.push_uint16(static_cast<uint16_t>(mapimage.width()));
	serialized.push_uint16(static_cast<uint16_t>(mapimage.height()));
	for (uint16_t i : metatilemap) {
		serialized.push_uint16(i);
	}

	elf.push_single_variable_rodata_sections({strings_name, STB_LOCAL}, strings.to_bytes());
	hostelf.push_single_variable_rodata_sections({strings_name, STB_LOCAL}, strings.to_bytes());
	elf.push_single_variable_rodata_sections({signs_name, STB_LOCAL}, signs.bytes_arm, signs.relocs_arm);
	hostelf.push_single_variable_rodata_sections({signs_name, STB_LOCAL}, signs.bytes_x8664, signs.relocs_x8664);
	elf.push_single_variable_rodata_sections({warps_name, STB_LOCAL}, warps.bytes_arm, warps.relocs_arm);
	hostelf.push_single_variable_rodata_sections({warps_name, STB_LOCAL}, warps.bytes_x8664, warps.relocs_x8664);
	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized.bytes_arm, serialized.relocs_arm);
	hostelf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized.bytes_x8664, serialized.relocs_x8664);
}

static void tile16x3map_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "enum WalkaroundBehavior {" << std::endl
		;
	for (auto wb = walkaround_behavior_names.begin(); wb != walkaround_behavior_names.end(); ++wb) {
		headerstream << "\t" << *wb << " = " <<
			(wb - walkaround_behavior_names.begin()) << "," << std::endl;
	}
	headerstream << "};" << std::endl
		<< "struct tile16x3 {" << std::endl
		<< "	enum WalkaroundBehavior behavior;" << std::endl
		<< "	bg_tile_t tiles[3][4];" << std::endl
		<< "};" << std::endl
		<< "#ifndef __unix__" << std::endl
		<< "_Static_assert(26 == sizeof(struct tile16x3));" << std::endl
		<< "#endif" << std::endl
		<< "struct sign_event {" << std::endl
		<< "	uint16_t x;" << std::endl
		<< "	uint16_t y;" << std::endl
		<< "	const char* message;" << std::endl
		<< "};" << std::endl
		<< "struct warp_event {" << std::endl
		<< "	uint16_t x;" << std::endl
		<< "	uint16_t y;" << std::endl
		<< "	struct {" << std::endl
		<< "		const struct tile16x3map* map;" << std::endl
		<< "		uint16_t warp;" << std::endl
		<< "	} destination;" << std::endl
		<< "};" << std::endl
		<< std::endl
		<< "struct tile16x3map {" << std::endl
		<< "	struct tileset tileset;" << std::endl
		<< "	const struct tile16x3* metatileset;" << std::endl
		<< "	const struct sign_event* signs;" << std::endl
		<< "	const struct warp_event* warps;" << std::endl
		<< "	uint16_t signs_count;" << std::endl
		<< "	uint16_t warps_count;" << std::endl
		<< "	uint16_t width;" << std::endl
		<< "	uint16_t height;" << std::endl
		<< "	uint16_t metatilemap[];" << std::endl
		<< "};" << std::endl;
}

const type_functions tile16x3map_type_functions(
	  &tile16x3map_write_struct
	, &tile16x3map_extract_palettes
	, &tile16x3map_extract_tiles
	, &tile16x3map_extract_metatiles
	, &tile16x3map_write_to_elf
);
