#include "background.hpp"

#include <array>
#include <bit>
#include <iostream>
#include <stdexcept>
#include "choose_compression.hpp"
#include "subword_output_iterator.hpp"
#include "variable_name_for_image.hpp"

bg_tile_t::bg_tile_t(uint16_t tile, bool hflip, bool vflip, uint16_t palette) : tile(tile), hflip(hflip), vflip(vflip), palette(palette) {}

std::array<uint8_t, 2> bg_tile_t::to_bytes(void) {
	std::array<uint8_t, 2> retval = {
		static_cast<uint8_t>(this->tile),
		static_cast<uint8_t>((this->tile >> 8) | (hflip ? 0x4 : 0) | (vflip ? 0x8 : 0) | (palette << 4)),
	};
	return retval;
}

uint16_t find_palette_superset(std::vector<std::array<rgba16_t, 16>> haystack, std::set<rgba16_t> needle) {
	uint16_t retval;
	for (retval = 0; retval < haystack.size(); ++retval) {
		const std::array<rgba16_t, 16> check_pal = haystack[retval];

		if (std::includes(check_pal.begin(), check_pal.end(), needle.begin(), needle.end())) {
			break;
		}
	}
	if (retval >= haystack.size()) {
		std::string msg("Lost this tile's palette");
		throw std::logic_error(msg);
	}
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

	std::vector<std::vector<uint8_t>> tileset;
	std::vector<bg_tile_t> tilemap;

	for (auto subimg : data.second.subs(8, 8)) {
		uint16_t pal_i = find_palette_superset(this->palette, subimg.palette());
		const std::array<rgba16_t, 16> used_pal = this->palette[pal_i];

		subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> tile1_builder;
		for (auto pixel : subimg.pixels()) {
			auto palptr = std::find(used_pal.begin(), used_pal.end(), pixel);
			uint4_t palindex(palptr - used_pal.begin());

			*tile1_builder = palindex;
			++tile1_builder;
		}
		std::vector<uint8_t> tile1(tile1_builder.result());

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
		for (auto item : tile1) {
			tileset_flat.push_back(item);
		}
	}

	this->tileset = tileset_flat;
	this->tilemap = tilemap;
}

void background::write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct background {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const char* tileset;" << std::endl
		<< "	const char* tilemap;" << std::endl
		<< "	const uint16_t palette_count;" << std::endl
		<< "	const uint16_t tileset_count;" << std::endl
		<< "	const uint16_t tilemap_count;" << std::endl
		<< "};" << std::endl;
}

void background::write(std::ostream& headerstream, struct Object* elf) const {
	headerstream << "extern struct background " << this->var_name << ";" << std::endl;

	std::array<uint16_t, 9> serialized = {
		0, 0,
		0, 0,
		0, 0,
		static_cast<uint16_t>(this->palette.size()),
		static_cast<uint16_t>(this->tileset.size() / 32),
		static_cast<uint16_t>(this->tilemap.size()),
	};

	char pal_name[32];
	snprintf(pal_name, 32, "%s.pal", this->var_name.c_str());
	char tileset_name[32];
	snprintf(tileset_name, 32, "%s.tileset", this->var_name.c_str());
	char tilemap_name[32];
	snprintf(tilemap_name, 32, "%s.tilemap", this->var_name.c_str());

	std::array<relocation_template, 3> relocs;
	relocs[0] = (struct relocation_template){
		.offset = 0,
		.type = R_ARM_ABS32,
		.symbol_name = pal_name,
	};
	relocs[1] = (struct relocation_template){
		.offset = 4,
		.type = R_ARM_ABS32,
		.symbol_name = tileset_name,
	};
	relocs[2] = (struct relocation_template){
		.offset = 8,
		.type = R_ARM_ABS32,
		.symbol_name = tilemap_name,
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


	object_push_bytes_section(elf, palettes_flat.data(), sizeof(rgba16_t) * palettes_flat.size(), {pal_name, STB_LOCAL});
	object_push_bytes_section(elf, tileset_comp.data.data(), sizeof(uint8_t) * tileset_comp.data.size(), {tileset_name, STB_LOCAL});
	object_push_bytes_section(elf, tilemap_comp.data.data(), sizeof(bg_tile_t) * tilemap_comp.data.size(), {tilemap_name, STB_LOCAL});

	object_push_bytes_section(elf, serialized.data(), sizeof(uint16_t) * serialized.size(), {this->var_name.c_str(), STB_GLOBAL});
	push_relocation_section(elf, this->var_name.c_str(), relocs.data(), relocs.size());
}
