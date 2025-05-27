#include "background.hpp"

#include <array>
#include <stdexcept>
#include "variable_name_for_image.hpp"

bg_tile_t::bg_tile_t(uint16_t tile) : tile(tile), hflip(false), vflip(false), palette(0) {}

void background::write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct background {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const tile_4bpp_t* tileset;" << std::endl
		<< "	const bg_tile_t* tilemap;" << std::endl
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

	object_push_bytes_section(elf, this->tileset.data(), sizeof(uint8_t) * this->tileset.size(), {tileset_name, STB_LOCAL});
	object_push_bytes_section(elf, this->tilemap.data(), sizeof(bg_tile_t) * this->tilemap.size(), {tilemap_name, STB_LOCAL});

	object_push_bytes_section(elf, serialized.data(), sizeof(uint32_t) * serialized.size(), {this->var_name.c_str(), STB_GLOBAL});
	push_relocation_section(elf, this->var_name.c_str(), relocs.data(), relocs.size());
}
