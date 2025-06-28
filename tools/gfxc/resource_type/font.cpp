#include "font.hpp"

#include <array>
#include "ints.hpp"
#include "variable_name_for_image.hpp"
#include "subword_output_iterator.hpp"

static const rgba16_t light = {31, 31, 31, 1};
static const rgba16_t shadow = {16, 16, 16, 1};
static const rgba16_t dark = {0, 0, 0, 1};
static const rgba16_t background = {0, 31, 0, 1};
static const rgba16_t outofbounds = {0, 16, 31, 1};


font::font(const std::pair<std::filesystem::path, bufferedimage> data)
	: var_name(variable_name_for_image(data))
{
	for (auto pixel : data.second.pixels()) {
		if (pixel == outofbounds)
			continue;
		if (pixel == background)
			continue;
		if (pixel == light)
			continue;
		if (pixel == shadow)
			continue;
		if (pixel == dark)
			continue;

		std::ostringstream msg;
		msg << data.first.string();
		msg << ": contained a color other than the allowed five: ";
		msg << pixel;
		throw std::logic_error(msg.str());
	}

	this->height = 0;
	for (auto subimg : data.second.subs(16, 16)) {
		for (unsigned y = height; y < 16; y++) {
			for (unsigned x = 0; x < 16; x++) {
				if (subimg.pixel(x,y) != outofbounds) {
					this->height = y + 1;
					break;
				}
			}
		}
	}

	for (auto subimg : data.second.subs(16, 16)) {
		font_glyph glyph;

		glyph.width = 0;
		for (unsigned y = 0; y < this->height; y++) {
			for (unsigned x = glyph.width; x < 16; x++) {
				if (subimg.pixel(x,y) != outofbounds) {
					glyph.width = x + 1;
				}
			}
		}

		subword_output_iterator<uint16_t, uint4_t, DIRECTION_INC> bits;

		subimage cropped = subimg.sub(0, 0, (((glyph.width - 1) / 4) + 1) * 4, this->height);
		for (auto pixel : cropped.pixels()) {
			if (pixel == background)
				*bits = 8_u4;
			else if (pixel == light)
				*bits = 1_u4;
			else if (pixel == shadow)
				*bits = 2_u4;
			else if (pixel == dark)
				*bits = 4_u4;
			else {
				*bits = 0_u4;
			}
			++bits;
		}

		glyph.data = bits.result();
		this->glyphs.push_back(glyph);
	}
}

void font::write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct font {" << std::endl
		<< "	const uint16_t* pixel_data;" << std::endl
		<< "	uint16_t glyph_height;" << std::endl
		<< "	uint16_t glyph_count;" << std::endl
		<< "	struct font_glyph {" << std::endl
		<< "		uint16_t width;" << std::endl
		<< "		uint16_t pixel_data_start_index;" << std::endl
		<< "	} glyphs[];" << std::endl
		<< "};" << std::endl;
}

void font::write(std::ostream& headerstream, Object& elf) const {
	headerstream << "extern const struct font " << this->var_name << ";" << std::endl;

	std::vector<uint16_t> pixel_data;
	std::vector<uint16_t> metadata;
	metadata.push_back(0);
	metadata.push_back(0);
	metadata.push_back(this->height);
	metadata.push_back(this->glyphs.size());
	for (font_glyph glyph : this->glyphs) {
		metadata.push_back(glyph.width);
		metadata.push_back(pixel_data.size());
		for (uint16_t b : glyph.data) {
			pixel_data.push_back(b);
		}
	}

	std::string pixeldata_name(this->var_name);
	pixeldata_name += ".pixeldata";

	elf.push_single_variable_rodata_sections({pixeldata_name, STB_LOCAL}, pixel_data);

	std::initializer_list<relocation_template> relocs {
		{
			.offset = 0,
			.type = R_ARM_ABS32,
			.symbol_name = pixeldata_name,
		}
	};

	elf.push_single_variable_rodata_sections({this->var_name, STB_GLOBAL}, metadata, relocs);
}
