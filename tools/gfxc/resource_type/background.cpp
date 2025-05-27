#include "background.hpp"

#include <array>
#include <bit>
#include <stdexcept>
#include <iostream>
#include "choose_compression.hpp"
#include "variable_name_for_image.hpp"

bg_tile_t::bg_tile_t(uint16_t tile) : tile(tile), hflip(false), vflip(false), palette(0) {}

std::array<uint8_t, 2> bg_tile_t::to_bytes(void) {
	std::array<uint8_t, 2> retval = {
		static_cast<uint8_t>(this->tile),
		static_cast<uint8_t>((this->tile >> 8) | (hflip ? 0x4 : 0) | (vflip ? 0x8 : 0) | (palette << 4)),
	};
	return retval;
}

void background::write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct background {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const char* tileset;" << std::endl
		<< "	const char* tilemap;" << std::endl
		<< "	const uint16_t tileset_count;" << std::endl
		<< "	const uint16_t tilemap_count;" << std::endl
		<< "};" << std::endl;
}

void background::write(std::ostream& headerstream, struct Object* elf) const {
	headerstream << "extern struct background " << this->var_name << ";" << std::endl;

	std::array<uint32_t, 4> serialized = {
		0,
		0,
		0,
		static_cast<uint32_t>((this->tileset.size() / 32) | ((this->tilemap.size()) << 16)),
	};

	char pal_name[16];
	snprintf(pal_name, 16, "plte.%x", this->paltag);
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

	auto tileset_comp = choose_compression(tileset_name, this->tileset);
	std::vector<uint8_t> tilemap_bytes;
	for (bg_tile_t entry : this->tilemap) {
		std::array<uint8_t, 2> bs = entry.to_bytes();
		tilemap_bytes.push_back(bs[0]);
		tilemap_bytes.push_back(bs[1]);
	}
	auto tilemap_comp = choose_compression(tilemap_name, tilemap_bytes);


	object_push_bytes_section(elf, tileset_comp.data.data(), sizeof(uint8_t) * tileset_comp.data.size(), {tileset_name, STB_LOCAL});
	object_push_bytes_section(elf, tilemap_comp.data.data(), sizeof(bg_tile_t) * tilemap_comp.data.size(), {tilemap_name, STB_LOCAL});

	object_push_bytes_section(elf, serialized.data(), sizeof(uint32_t) * serialized.size(), {this->var_name.c_str(), STB_GLOBAL});
	push_relocation_section(elf, this->var_name.c_str(), relocs.data(), relocs.size());
}
