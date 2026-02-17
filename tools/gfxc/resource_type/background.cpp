#include "background.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include "choose_compression.hpp"
#include "find_palette_superset.hpp"
#include "object.hpp"
#include "resource_type/tileset.hpp"

// may improve frit compression
static constexpr bool SORT_BY_PALETTE = true;

std::ostream& operator<<(std::ostream& os, const std::set<rgba16_t>& value) {
	std::ios_base::fmtflags initial_flags = os.flags();
	auto initial_width = os.width();
	auto initial_fill = os.fill();

	for (rgba16_t c : value) {
		os << "#"
			<< std::setw(2) << std::setfill('0') << std::hex << c.r
			<< std::setw(2) << std::setfill('0') << std::hex << c.g
			<< std::setw(2) << std::setfill('0') << std::hex << c.b
			<< c.a << ", ";
	}

	os.fill(initial_fill);
	os.width(initial_width);
	os.flags(initial_flags);
	return os;
}

std::ostream& operator<<(std::ostream& os, const std::array<rgba16_t, 16>& value) {
	std::ios_base::fmtflags initial_flags = os.flags();
	auto initial_width = os.width();
	auto initial_fill = os.fill();

	for (rgba16_t c : value) {
		os << "#"
			<< std::setw(2) << std::setfill('0') << std::hex << c.r
			<< std::setw(2) << std::setfill('0') << std::hex << c.g
			<< std::setw(2) << std::setfill('0') << std::hex << c.b
			<< c.a << ", ";
	}

	os.fill(initial_fill);
	os.width(initial_width);
	os.flags(initial_flags);
	return os;
}

std::vector<gbatile_4bpp> background_extract_tiles(input_path_and_data input, palette_data palettes) {
	std::vector<gbatile_4bpp> retval;
	bufferedimage image = std::get<bufferedimage>(input.second);

	if (SORT_BY_PALETTE) {
		std::vector<std::vector<gbatile_4bpp>> tileset_per_palette;

		for (size_t i = 0; i < palettes.colorss.size(); i++)
			tileset_per_palette.emplace_back();

		for (auto subimg : image.subs(8, 8)) {
			uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
			const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

			const gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));
			const gbatile_4bpp_matcher tile1m(tile1);

			auto tileset_contains_tile1 = [tile1m](std::vector<gbatile_4bpp>& check_tileset) {
				return check_tileset.end() != std::find_if(check_tileset.begin(), check_tileset.end(), tile1m);
			};

			if (tileset_per_palette.end() == std::find_if(tileset_per_palette.begin(), tileset_per_palette.end(), tileset_contains_tile1)) {
				tileset_per_palette[pal_i].push_back(tile1);
			}
		}

		for (auto tiles : tileset_per_palette) {
			for (auto tile : tiles) {
				retval.push_back(tile);
			}
		}

	} else {
		for (auto subimg : image.subs(8, 8)) {
			uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
			const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

			const gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));
			const gbatile_4bpp_matcher tile1m(tile1);

			if (retval.end() == std::find_if(retval.begin(), retval.end(), tile1m)) {
				retval.push_back(tile1);
			}
		}
	}

	return retval;
}

std::vector<bg_tile_t> background_extract_map(input_path_and_data input, palette_data palettes, tiles_data tiles_dat) {
	std::vector<bg_tile_t> retval;
	bufferedimage image = std::get<bufferedimage>(input.second);

	const std::vector<gbatile_4bpp> tiles(tiles_dat.tiles);

	for (auto subimg : image.subs(8, 8)) {
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
		for (tile_i = 0; tile_i < tiles.size(); tile_i++) {
			if (tile1 == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, false, false, pal_i));
				break;
			}
			if (tile1h == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, true, false, pal_i));
				break;
			}
			if (tile1v == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, false, true, pal_i));
				break;
			}
			if (tile1hv == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, true, true, pal_i));
				break;
			}
		}
		if (tile_i >= tiles.size()) {
			std::string msg(input.first);
			msg += ": lost tile between `background_extract_tiles` and `background_write_to_elf`";
			throw std::logic_error(msg);
		}
	}

	return retval;
}

static void background_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct background {" << std::endl
		<< "	struct tileset tileset;" << std::endl
		<< "	const struct CompressedData* tilemap;" << std::endl
		<< "	uint16_t tilemap_count;" << std::endl
		<< "};" << std::endl;
}

static void background_write_to_elf(
	input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles_pair,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	[[maybe_unused]] Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const struct background " << var_name << ";" << std::endl;

	std::vector<bg_tile_t> tilemap = background_extract_map(input, palettes.second, tiles_pair.second);

	std::string pal_name = palettes.first;
	std::string tileset_name = tiles_pair.first;
	std::string tilemap_name("map.");
	tilemap_name += var_name;

	std::array<uint16_t, 12> serialized = {0};
	std::array<relocation_template, 3> relocs = {0};

	tileset_serialized(
		std::span<uint16_t, 8>(serialized.begin(), 8),
		std::span<relocation_template, 2>(relocs.begin(), 2),
		palettes,
		tiles_pair);
	serialized[10] = static_cast<uint16_t>(tilemap.size());
	relocs[2] = {
		.offset = 16,
		.type = R_ARM_ABS32,
		.symbol_name = tilemap_name,
	};

	std::vector<uint8_t> tilemap_bytes;
	for (bg_tile_t entry : tilemap) {
		std::array<uint8_t, 2> bs = entry.to_bytes();
		tilemap_bytes.push_back(bs[0]);
		tilemap_bytes.push_back(bs[1]);
	}
	auto tilemap_comp = choose_compression(tilemap_name, tilemap_bytes);

	elf.push_single_variable_rodata_sections({tilemap_name, STB_LOCAL}, tilemap_comp.data);
	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized, relocs);
}

const type_functions background_type_functions(
	  &background_write_struct
	, &tileset_extract_palettes
	, &background_extract_tiles
	, nullptr
	, &background_write_to_elf
);
