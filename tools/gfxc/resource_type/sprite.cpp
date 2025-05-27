#include "sprite.hpp"

#include <array>
#include <stdexcept>
#include "variable_name_for_image.hpp"

enum sprite_size sprite_size(unsigned width, unsigned height) {
	if (8 == width && 8 == height)
		return SIZE_8x8;
	if (16 == width && 16 == height)
		return SIZE_16x16;
	if (32 == width && 32 == height)
		return SIZE_32x32;
	if (64 == width && 64 == height)
		return SIZE_64x64;
	if (16 == width && 8 == height)
		return SIZE_16x8;
	if (32 == width && 8 == height)
		return SIZE_32x8;
	if (32 == width && 16 == height)
		return SIZE_32x16;
	if (64 == width && 32 == height)
		return SIZE_64x32;
	if (8 == width && 16 == height)
		return SIZE_8x16;
	if (8 == width && 32 == height)
		return SIZE_8x32;
	if (16 == width && 32 == height)
		return SIZE_16x32;
	if (32 == width && 64 == height)
		return SIZE_32x64;

	throw std::invalid_argument("invalid sprite size");
}

std::ostream& operator<<(std::ostream& os, enum sprite_size v) {
	switch (v) {
	case SIZE_8x8:
		os << "8x8";
		break;
	case SIZE_16x16:
		os << "16x16";
		break;
	case SIZE_32x32:
		os << "32x32";
		break;
	case SIZE_64x64:
		os << "64x64";
		break;
	case SIZE_16x8:
		os << "16x8";
		break;
	case SIZE_32x8:
		os << "32x8";
		break;
	case SIZE_32x16:
		os << "32x16";
		break;
	case SIZE_64x32:
		os << "64x32";
		break;
	case SIZE_8x16:
		os << "8x16";
		break;
	case SIZE_8x32:
		os << "8x32";
		break;
	case SIZE_16x32:
		os << "16x32";
		break;
	case SIZE_32x64:
		os << "32x64";
		break;
	}
	return os;
}


void sprite::write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "typedef uint16_t paltag_t;" << std::endl
		<< "typedef uint16_t tiletag_t;" << std::endl
		<< "struct shadow_oam_template {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const char* tiles;" << std::endl
		<< "	paltag_t paltag;" << std::endl
		<< "	tiletag_t tiletag;" << std::endl
		<< "	enum oam_shape shape: 2;" << std::endl
		<< "	uint8_t size : 2;" << std::endl
		<< "};" << std::endl;
}

void sprite::write(std::ostream& headerstream, struct Object* elf) const {
	headerstream << "extern const struct shadow_oam_template " << this->var_name << ";" << std::endl;

	std::array<uint32_t, 4> serialized = {
		0,
		0,
		static_cast<uint32_t>(this->paltag | (this->tiletag << 16)),
		this->size,
	};

	char pal_name[16];
	snprintf(pal_name, 16, "plte.%x", this->paltag);
	char tiles_name[16];
	snprintf(tiles_name, 16, "tile.%x", this->tiletag);

	std::array<relocation_template, 2> relocs;
	relocs[0] = (struct relocation_template){
		.offset = 0,
		.type = R_ARM_ABS32,
		.symbol_name = pal_name,
	};
	relocs[1] = (struct relocation_template){
		.offset = 4,
		.type = R_ARM_ABS32,
		.symbol_name = tiles_name,
	};

	object_push_bytes_section(elf, serialized.data(), sizeof(uint32_t) * serialized.size(), {this->var_name.c_str(), STB_GLOBAL});
	push_relocation_section(elf, this->var_name.c_str(), relocs.data(), relocs.size());
}
