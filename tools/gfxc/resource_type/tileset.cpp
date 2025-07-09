#include "tileset.hpp"

#include <iostream>
#include <stdexcept>
#include "choose_compression.hpp"
#include "variable_name_for_image.hpp"

tileset::tileset(const std::pair<std::filesystem::path, bufferedimage> data)
	: var_name(variable_name_for_image(data))
{
	{
		std::set<rgba16_t> palette_set = data.second.palette();
		palette_set.insert(data.second.background().with_alpha(0));
		if (palette_set.size() > this->palette.size()) {
			std::string msg(data.first.string());
			msg += ": tileset palette larger than 16 colors";
			throw std::logic_error(msg);
		}

		unsigned index = 0;
		for (auto it = palette_set.begin(); it != palette_set.end(); ++it, ++index) {
			this->palette[index] = *it;
		}
		for (; index != palette_set.size(); ++index) {
			this->palette[index] = rgba16::BLACK;
		}
	}

	for (auto subimg : data.second.subs(8, 8)) {
		gbatile_4bpp tile1(subimg.to_tile_4bpp(this->palette));

		for (auto b : tile1.bytes()) {
			this->tiles.push_back(b);
		}
	}
}

void tileset::write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct tileset {" << std::endl
		<< "	const palette16_t* palette;" << std::endl
		<< "	const struct CompressedData* tileset;" << std::endl
		<< "	const uint16_t tileset_count;" << std::endl
		<< "};" << std::endl;
}

void tileset::write(std::ostream& headerstream, Object& elf) const {
	headerstream << "extern const struct tileset " << this->var_name << ";" << std::endl;

	std::array<uint16_t, 6> serialized = {
		0, 0,
		0, 0,
		static_cast<uint16_t>(this->tiles.size() / 32),
		0,
	};

	std::string pal_name(this->var_name);
	pal_name += ".pal";
	std::string tileset_name(this->var_name);
	tileset_name += ".tileset";

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
	};

	auto tileset_comp = choose_compression(tileset_name, this->tiles);


	elf.push_single_variable_rodata_sections({pal_name, STB_LOCAL}, this->palette);
	elf.push_single_variable_rodata_sections({tileset_name, STB_LOCAL}, tileset_comp.data);

	elf.push_single_variable_rodata_sections({this->var_name, STB_GLOBAL}, serialized, relocs);
}
