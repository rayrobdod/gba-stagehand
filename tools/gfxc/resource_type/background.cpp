#include "background.hpp"

#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>
#include "choose_compression.hpp"
#include "find_palette_superset.hpp"
#include "subword_output_iterator.hpp"
#include "variable_name_for_image.hpp"
#include "resource_type/tileset.hpp"

// may improve frit compression
static constexpr bool SORT_BY_PALETTE = true;

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


background::background(const std::pair<std::filesystem::path, bufferedimage> data)
	: var_name(variable_name_for_image(data))
{
	std::set<std::set<rgba16_t>> tile_palettes2;
	for (auto subimg : data.second.subs(8, 8)) {
		std::set<rgba16_t> new_pal;
		new_pal.insert(data.second.background().with_alpha(0));
		new_pal.merge(subimg.palette());
		if (new_pal.size() > 16) {
			std::string msg(data.first.string());
			msg += ": tile palette larger than 16 colors";
			throw std::logic_error(msg);
		}
		tile_palettes2.insert(new_pal);
	}

	std::vector<std::set<rgba16_t>> tile_palettes(tile_palettes2.begin(), tile_palettes2.end());
	std::sort(tile_palettes.begin(), tile_palettes.end(),
		[](std::set<rgba16_t> a, std::set<rgba16_t> b) {
			return a.size() > b.size();
		}
	);

	std::vector<std::set<rgba16_t>> palettes_builder;
	for (std::set<rgba16_t> tile_pal : tile_palettes) {
		unsigned pal_i;
		for (pal_i = 0; pal_i < palettes_builder.size(); pal_i++) {
			std::set<rgba16_t> tile_pal2(tile_pal.begin(), tile_pal.end());

			std::set<rgba16_t> new_pal(palettes_builder[pal_i].begin(), palettes_builder[pal_i].end());
			new_pal.merge(tile_pal2);

			if (new_pal.size() <= 16) {
				palettes_builder[pal_i] = new_pal;
				break;
			}
		}
		if (pal_i >= palettes_builder.size()) {
			palettes_builder.push_back(tile_pal);
		}
	}

	if (palettes_builder.size() > 16) {
		std::string msg(data.first.string());
		msg += ": image has too many palettes";
		throw std::logic_error(msg);
	}

	for (std::set<rgba16_t> pal : palettes_builder) {
		std::vector pal2(pal.begin(), pal.end());
		std::array<rgba16_t, 16> newpal;
		size_t i;
		for (i = 0; i < pal2.size(); i++) {
			newpal[i] = pal2[i];
		}
		for (; i < 16; i++) {
			newpal[i] = rgba16::BLACK;
		}
		this->palette.push_back(newpal);
	}

	std::vector<std::vector<gbatile_4bpp>> tileset_per_palette;
	if (SORT_BY_PALETTE) {
		for (size_t i = 0; i < this->palette.size(); i++)
			tileset_per_palette.emplace_back();

		for (auto subimg : data.second.subs(8, 8)) {
			uint16_t pal_i = find_palette_superset<std::vector<std::array<rgba16_t, 16>>>(this->palette, subimg.palette());
			const std::array<rgba16_t, 16> used_pal = this->palette[pal_i];

			gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

			unsigned i;
			for (i = 0; i < tileset_per_palette[pal_i].size(); i++) {
				if (tile1 == tileset_per_palette[pal_i][i]) {
					break;
				}
			}
			if (i >= tileset_per_palette[pal_i].size()) {
				tileset_per_palette[pal_i].push_back(tile1);
			}
		}
	}

	std::vector<gbatile_4bpp> tileset;
	std::vector<bg_tile_t> tilemap;

	if (SORT_BY_PALETTE) {
		for (auto tileset1 : tileset_per_palette)
		for (auto tile1 : tileset1) {
			unsigned i;
			for (i = 0; i < tileset.size(); i++) {
				if (tile1 == tileset[i]) {
					break;
				}
			}
			if (i >= tileset.size()) {
				tileset.push_back(tile1);
			}
		}
	}

	for (auto subimg : data.second.subs(8, 8)) {
		uint16_t pal_i = find_palette_superset<std::vector<std::array<rgba16_t, 16>>>(this->palette, subimg.palette());
		const std::array<rgba16_t, 16> used_pal = this->palette[pal_i];

		gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

		unsigned i;
		for (i = 0; i < tileset.size(); i++) {
			if (tile1 == tileset[i]) {
				tilemap.push_back(bg_tile_t(i, false, false, pal_i));
				break;
			}
		}
		if (i >= tileset.size()) {
			tileset.push_back(tile1);
			tilemap.push_back(bg_tile_t(i, false, false, pal_i));
		}
	}

	std::vector<uint8_t> tileset_flat;
	for (auto tile1 : tileset) {
		for (auto item : tile1.bytes()) {
			tileset_flat.push_back(item);
		}
	}

	this->tileset = tileset_flat;
	this->tilemap = tilemap;
}

void background::write(std::ostream& headerstream, Object& elf) const {
	headerstream << "extern struct background " << this->var_name << ";" << std::endl;

	std::array<uint16_t, 9> serialized = {
		0, 0,
		0, 0,
		0, 0,
		static_cast<uint16_t>(this->palette.size()),
		static_cast<uint16_t>(this->tileset.size() / 32),
		static_cast<uint16_t>(this->tilemap.size()),
	};

	std::string pal_name(this->var_name);
	pal_name += ".pal";
	std::string tileset_name(this->var_name);
	tileset_name += ".tileset";
	std::string tilemap_name(this->var_name);
	tilemap_name += ".tilemap";

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

	std::vector<rgba16_t> palettes_flat;
	for (auto row : this->palette) {
		for (auto item : row) {
			palettes_flat.push_back(item);
		}
	}

	auto tileset_comp = choose_compression(tileset_name, this->tileset);
	std::vector<uint8_t> tilemap_bytes;
	for (bg_tile_t entry : this->tilemap) {
		std::array<uint8_t, 2> bs = entry.to_bytes();
		tilemap_bytes.push_back(bs[0]);
		tilemap_bytes.push_back(bs[1]);
	}
	auto tilemap_comp = choose_compression(tilemap_name, tilemap_bytes);


	elf.push_single_variable_rodata_sections({pal_name, STB_LOCAL}, palettes_flat);
	elf.push_single_variable_rodata_sections({tileset_name, STB_LOCAL}, tileset_comp.data);
	elf.push_single_variable_rodata_sections({tilemap_name, STB_LOCAL}, tilemap_comp.data);

	elf.push_single_variable_rodata_sections({this->var_name, STB_GLOBAL}, serialized, relocs);
}

static std::vector<gbatile_4bpp> background_extract_tiles(std::pair<std::filesystem::path, struct bufferedimage> image, palette_data palettes) {
	std::vector<gbatile_4bpp> retval;

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

	std::vector<bg_tile_t> tilemap;
	const std::vector<gbatile_4bpp> tiles = tiles_pair.second.tiles;

	for (auto subimg : image.second.subs(8, 8)) {
		uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.second.colorss, subimg.palette());
		const std::vector<rgba16_t> used_pal = palettes.second.colorss[pal_i];

		gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

		unsigned tile_i;
		for (tile_i = 0; tile_i < tiles.size(); tile_i++) {
			if (tile1 == tiles[tile_i]) {
				tilemap.push_back(bg_tile_t(tile_i, false, false, pal_i));
				break;
			}
		}
		if (tile_i >= tiles.size()) {
			std::string msg(image.first);
			msg += ": lost tile between `background_extract_tiles` and `background_write_to_elf`";
			throw std::logic_error(msg);
		}
	}

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
