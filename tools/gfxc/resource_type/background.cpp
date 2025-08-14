#include "background.hpp"

#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>
#include "choose_compression.hpp"
#include "find_palette_superset.hpp"
#include "object.hpp"
#include "resource_type/tileset.hpp"

// may improve frit compression
static constexpr bool SORT_BY_PALETTE = true;

bg_tile_t::bg_tile_t() : tile(0), hflip(false), vflip(false), palette(0) {}
bg_tile_t::bg_tile_t(uint16_t tile, bool hflip, bool vflip, uint16_t palette) : tile(tile), hflip(hflip), vflip(vflip), palette(palette) {}

std::array<uint8_t, 2> bg_tile_t::to_bytes(void) {
	std::array<uint8_t, 2> retval = {
		static_cast<uint8_t>(this->tile),
		static_cast<uint8_t>((this->tile >> 8) | (hflip ? 0x4 : 0) | (vflip ? 0x8 : 0) | (palette << 4)),
	};
	return retval;
}

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

std::vector<gbatile_4bpp> background_extract_tiles(std::pair<std::filesystem::path, struct bufferedimage> image, palette_data palettes) {
	std::vector<gbatile_4bpp> retval;

	if (SORT_BY_PALETTE) {
		std::vector<std::vector<gbatile_4bpp>> tileset_per_palette;

		for (size_t i = 0; i < palettes.colorss.size(); i++)
			tileset_per_palette.emplace_back();

		for (auto subimg : image.second.subs(8, 8)) {
			uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
			const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

			gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

			auto tileset_contains_tile1 = [tile1](std::vector<gbatile_4bpp>& check_tileset) {
				return check_tileset.end() != std::find(check_tileset.begin(), check_tileset.end(), tile1);
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
		for (auto subimg : image.second.subs(8, 8)) {
			uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
			const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

			gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

			unsigned i;
			for (i = 0; i < retval.size(); i++) {
				if (tile1 == retval[i]) {
					break;
				}
			}
			if (i >= retval.size()) {
				retval.push_back(tile1);
			}
		}
	}

	return retval;
}

std::vector<bg_tile_t> background_extract_map(std::pair<std::filesystem::path, struct bufferedimage> image, palette_data palettes, tiles_data tiles_dat) {
	std::vector<bg_tile_t> retval;

	const std::vector<gbatile_4bpp> tiles = tiles_dat.tiles;

	for (auto subimg : image.second.subs(8, 8)) {
		uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
		const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

		gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

		unsigned tile_i;
		for (tile_i = 0; tile_i < tiles.size(); tile_i++) {
			if (tile1 == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, false, false, pal_i));
				break;
			}
		}
		if (tile_i >= tiles.size()) {
			std::string msg(image.first);
			msg += ": lost tile between `background_extract_tiles` and `background_write_to_elf`";
			throw std::logic_error(msg);
		}
	}

	return retval;
}

static void background_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct background {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const struct CompressedData* tileset;" << std::endl
		<< "	const struct CompressedData* tilemap;" << std::endl
		<< "	const uint16_t palette_count;" << std::endl
		<< "	const uint16_t tileset_count;" << std::endl
		<< "	const uint16_t tilemap_count;" << std::endl
		<< "};" << std::endl;
}

static void background_write_to_elf(
	[[gnu::unused]] std::pair<std::filesystem::path, struct bufferedimage> image,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles_pair,
	std::string var_name,
	std::ostream& headerstream,
	Object& elf
) {
	headerstream << "extern struct background " << var_name << ";" << std::endl;

	std::vector<bg_tile_t> tilemap = background_extract_map(image, palettes.second, tiles_pair.second);

	std::array<uint16_t, 9> serialized = {
		0, 0,
		0, 0,
		0, 0,
		static_cast<uint16_t>(palettes.second.colorss.size()),
		static_cast<uint16_t>(tiles_pair.second.tiles.size()),
		static_cast<uint16_t>(tilemap.size()),
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
	, &background_write_to_elf
);
