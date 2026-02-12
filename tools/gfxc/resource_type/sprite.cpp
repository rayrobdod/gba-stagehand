#include "sprite.hpp"

#include <array>
#include <stdexcept>
#include "object.hpp"

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

static palette_data_builder sprite_extract_palettes(input_path_and_data input) {
	palette_data_builder retval;

	bufferedimage image = std::get<bufferedimage>(input.second);
	std::set<rgba16_t> colors;
	colors.insert(image.background().with_alpha(0));
	colors.merge(image.palette());

	if (colors.size() > 16) {
		std::string msg(input.first);
		msg += ": sprite palette larger than 16 colors";
		throw std::logic_error(msg);
	}

	retval.colorss.insert(colors);

	retval.alternates = image.alt_palettes();

	return retval;
}

static std::vector<gbatile_4bpp> sprite_extract_tiles(input_path_and_data input, palette_data palettes) {
	if (1 != palettes.colorss.size()) {
		std::string msg(input.first);
		msg += ": more than one palette for sprite";
		throw std::logic_error(msg);
	}

	const std::vector<rgba16_t> used_pal = palettes.colorss[0];
	std::vector<gbatile_4bpp> retval;

	bufferedimage image = std::get<bufferedimage>(input.second);
	for (auto subimg : image.subs(8, 8)) {
		gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));

		retval.push_back(tile1);
	}

	return retval;
}

static void sprite_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "typedef uint16_t paltag_t;" << std::endl
		<< "typedef uint16_t tiletag_t;" << std::endl
		<< "struct shadow_oam_template {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const struct CompressedData* tiles;" << std::endl
		<< "	paltag_t paltag;" << std::endl
		<< "	tiletag_t tiletag;" << std::endl
		<< "	enum oam_shape shape: 2;" << std::endl
		<< "	uint8_t size : 2;" << std::endl
		<< "};" << std::endl;
}

static void sprite_write_to_elf(
	input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	[[maybe_unused]] Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const struct shadow_oam_template " << var_name << ";" << std::endl;

	bufferedimage image = std::get<bufferedimage>(input.second);

	enum sprite_size my_size = sprite_size(image.width(), image.height());

	std::array<uint16_t, 8> serialized = {
		0, 0,
		0, 0,
		palettes.second.tag,
		tiles.second.tag,
		my_size,
		0,
	};

	std::vector<relocation_template> relocs {
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
	};

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized, relocs);

	for (auto alternate : palettes.second.alternates) {
		std::string alternate_var_name;
		alternate_var_name += var_name;
		alternate_var_name += "_";
		alternate_var_name += alternate.first;

		headerstream << "extern const struct shadow_oam_template " << alternate_var_name << ";" << std::endl;

		std::string alternate_palette_name;
		alternate_palette_name += palettes.first;
		alternate_palette_name += "_";
		alternate_palette_name += alternate.first;

		serialized[4] = alternate.second.tag;
		relocs[0].symbol_name = alternate_palette_name;

		elf.push_single_variable_rodata_sections({alternate_var_name, STB_GLOBAL}, serialized, relocs);
	}
}

const type_functions sprite_type_functions(
	  &sprite_write_struct
	, &sprite_extract_palettes
	, &sprite_extract_tiles
	, nullptr
	, &sprite_write_to_elf
);
