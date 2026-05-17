#include "font.hpp"

#include <array>
#include "ints.hpp"
#include "object.hpp"
#include "subword_output_iterator.hpp"

static const rgba16_t light = {31, 31, 31, 1};
static const rgba16_t shadow = {16, 16, 16, 1};
static const rgba16_t dark = {0, 0, 0, 1};
static const rgba16_t background = {0, 31, 0, 1};
static const rgba16_t outofbounds = {0, 16, 31, 1};

static std::vector<uint8_t> utf8(size_t c) {
	std::vector<uint8_t> retval;
	if (c < 0x80) {
		retval.push_back(static_cast<uint8_t>(c));
	}
	else if (c < 0x800) {
		retval.push_back(static_cast<uint8_t>(0xC0 | (c >> 6)));
		retval.push_back(static_cast<uint8_t>(0x80 | (c & 0x3F)));
	}
	else if (c < 0x10000) {
		retval.push_back(static_cast<uint8_t>(0xE0 | (c >> 12)));
		retval.push_back(static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3F)));
		retval.push_back(static_cast<uint8_t>(0x80 | (c & 0x3F)));
	}
	else if (c < 0x110000) {
		retval.push_back(static_cast<uint8_t>(0xF0 | (c >> 18)));
		retval.push_back(static_cast<uint8_t>(0x80 | ((c >> 12) & 0x3F)));
		retval.push_back(static_cast<uint8_t>(0x80 | ((c >> 6) & 0x3F)));
		retval.push_back(static_cast<uint8_t>(0x80 | (c & 0x3F)));
	}
	return retval;
}

struct font_glyph {
	uint8_t width;
	std::vector<uint16_t> data;
};


struct byte_to_glyph_trie_allocated {
	uint8_t start;
	uint8_t end;
	uint16_t glyphs_start;
	uint16_t links_start;

	byte_to_glyph_trie_allocated() : start(0xFF), end(0xFF), glyphs_start(0xFFFF), links_start(0) {}
};

class byte_to_glyph_trie_compact {
public:
	struct link {
		uint8_t start;
		uint8_t end;
		uint16_t glyphs_start;
		std::vector<byte_to_glyph_trie_compact::link> links;

	public:
		link(uint8_t _start, uint8_t _end, uint16_t _glyphs_start) :
			start(_start), end(_end), glyphs_start(_glyphs_start) {}
		link(uint8_t _start, uint8_t _end, uint16_t _glyphs_start, std::vector<byte_to_glyph_trie_compact::link> _links) :
			start(_start), end(_end), glyphs_start(_glyphs_start), links(_links) {}

		void serialize_to(std::vector<byte_to_glyph_trie_allocated>& retval, size_t my_pos) const {
			retval[my_pos].start = this->start;
			retval[my_pos].end = this->end;
			retval[my_pos].glyphs_start = this->glyphs_start;
			if (this->links.empty()) {
				retval[my_pos].links_start = 0;
			} else {
				size_t start_links_pos = retval.size();

				retval[my_pos].links_start = start_links_pos;
				retval.resize(retval.size() + this->links.size() + 1);

				for (size_t i = 0; i < this->links.size(); ++i) {
					this->links[i].serialize_to(retval, start_links_pos + i);
				}
			}
		}
	};
private:
	std::vector<byte_to_glyph_trie_compact::link> links;
public:
	byte_to_glyph_trie_compact(std::vector<byte_to_glyph_trie_compact::link> _links) : links(_links) {}

	std::vector<byte_to_glyph_trie_allocated> serialize() const {
		std::vector<byte_to_glyph_trie_allocated> retval;
		retval.resize(this->links.size() + 1);

		for (size_t i = 0; i < this->links.size(); ++i) {
			this->links[i].serialize_to(retval, i);
		}
		return retval;
	}
};

class byte_to_glyph_trie {
private:
	std::optional<uint16_t> value;
	std::vector<byte_to_glyph_trie> links;
public:
	byte_to_glyph_trie() {}

	void emplace(const std::vector<uint8_t>& key, uint16_t value) {
		if (key.empty()) {
			if (!this->value) {
				this->value.emplace(value);
			}
		}
		else {
			uint8_t key_head = key.front();
			std::vector<uint8_t> key_tail(key.begin() + 1, key.end());

			links.resize(256);
			links[key_head].emplace(key_tail, value);
		}
	}

	byte_to_glyph_trie_compact compact() const {
		byte_to_glyph_trie_compact retval(this->compact_0());
		return retval;
	}

	std::vector<byte_to_glyph_trie_compact::link> compact_0() const {
		std::vector<byte_to_glyph_trie_compact::link> retval;

		size_t i = 0;
		while (i < 256) {
			if (! this->links[i].value && this->links[i].links.empty()) {
				++i;
			}
			else if (! this->links[i].links.empty()) {
				retval.emplace_back(static_cast<uint8_t>(i), static_cast<uint8_t>(i), this->links[i].value.value_or(0xFFFF), this->links[i].compact_0());
				++i;
			}
			else {
				uint8_t start_byte = i;
				uint16_t start_value = *(this->links[i].value);
				uint16_t current_value = start_value;
				do {
					++i;
					++current_value;
				} while (i < 256 && this->links[i].links.empty() && this->links[i].value && current_value == *(this->links[i].value));

				retval.emplace_back(start_byte, static_cast<uint8_t>(i - 1), start_value);
			}
		}

		return retval;
	}
};



static void font_write_to_elf(
	input_path_and_data input,
	[[gnu::unused]] std::pair<std::string, palette_data> palettes,
	[[gnu::unused]] std::pair<std::string, tiles_data> tiles,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	Object_x8664& hostelf,
	Object& elf
) {
	uint8_t height = 0;

	bufferedimage image = std::get<bufferedimage>(input.second);

	for (auto pixel : image.pixels()) {
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
		msg << input.first.string();
		msg << ": contained a color other than the allowed five: ";
		msg << pixel;
		throw std::logic_error(msg.str());
	}

	for (auto subimg : image.subs(16, 16)) {
		for (unsigned y = height; y < 16; y++) {
			for (unsigned x = 0; x < 16; x++) {
				if (subimg.pixel(x,y) != outofbounds) {
					height = y + 1;
					break;
				}
			}
		}
	}

	std::vector<font_glyph> glyphs;
	for (auto subimg : image.subs(16, 16)) {
		font_glyph glyph;

		glyph.width = 0;
		for (unsigned y = 0; y < height; y++) {
			for (unsigned x = glyph.width; x < 16; x++) {
				if (subimg.pixel(x,y) != outofbounds) {
					glyph.width = x + 1;
				}
			}
		}

		subword_output_iterator<uint16_t, uint4_t, DIRECTION_INC> bits;

		subimage cropped = subimg.sub(0, 0, ((glyph.width / 4) * 4) + (glyph.width % 4 ? 4 : 0), height);
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
		glyphs.push_back(glyph);
	}

	byte_to_glyph_trie byte_map_wide;
	{
		size_t glyph_i = 0;
		size_t char_i = ' ';

		while (glyph_i < glyphs.size()) {
			if (glyphs[glyph_i].width == 0) {
				glyphs.erase(glyphs.begin() + glyph_i);
			} else {
				const std::vector<uint8_t> bytes = utf8(char_i);
				byte_map_wide.emplace(bytes, glyph_i);
				glyph_i++;
			}

			char_i++;
		}
	}

	byte_to_glyph_trie_compact byte_map_compact = byte_map_wide.compact();
	std::vector<byte_to_glyph_trie_allocated> byte_map_serialized = byte_map_compact.serialize();


	headerstream << "extern const struct font " << var_name << ";" << std::endl;

	std::vector<uint16_t> pixel_data;
	std::vector<uint16_t> glyph_data;
	for (auto glyph : glyphs) {
		glyph_data.push_back(glyph.width);
		glyph_data.push_back(pixel_data.size());
		for (uint16_t b : glyph.data) {
			pixel_data.push_back(b);
		}
	}

	std::vector<uint16_t> metadata;
	for (unsigned i = 0; i < 3 * 2; i++) {
		metadata.push_back(0);
	}
	metadata.push_back(height);

	std::vector<uint16_t> metadata_x8664;
	for (unsigned i = 0; i < 3 * 2; i++) {
		metadata_x8664.push_back(0);
	}
	std::copy(metadata.begin(), metadata.end(), std::back_inserter(metadata_x8664));

	std::string pixeldata_name("pixeldata.");
	pixeldata_name += var_name;

	std::string glyphdata_name("glyphdata.");
	glyphdata_name += var_name;

	std::string glyphmapping_name("glyphmapping.");
	glyphmapping_name += var_name;

	elf.push_single_variable_rodata_sections({pixeldata_name, STB_LOCAL}, pixel_data);
	hostelf.push_single_variable_rodata_sections({pixeldata_name, STB_LOCAL}, pixel_data);

	elf.push_single_variable_rodata_sections({glyphdata_name, STB_LOCAL}, glyph_data);
	hostelf.push_single_variable_rodata_sections({glyphdata_name, STB_LOCAL}, glyph_data);

	elf.push_single_variable_rodata_sections({glyphmapping_name, STB_LOCAL}, byte_map_serialized);
	hostelf.push_single_variable_rodata_sections({glyphmapping_name, STB_LOCAL}, byte_map_serialized);

	std::initializer_list<relocation_template> relocs {
		{
			.offset = 0,
			.type = R_ARM_ABS32,
			.symbol_name = pixeldata_name,
		},
		{
			.offset = 4,
			.type = R_ARM_ABS32,
			.symbol_name = glyphdata_name,
		},
		{
			.offset = 8,
			.type = R_ARM_ABS32,
			.symbol_name = glyphmapping_name,
		},
	};
	std::initializer_list<relocation_template> relocs_x8664 {
		{
			.offset = 0,
			.type = R_X86_64_64,
			.symbol_name = pixeldata_name,
		},
		{
			.offset = 8,
			.type = R_X86_64_64,
			.symbol_name = glyphdata_name,
		},
		{
			.offset = 16,
			.type = R_X86_64_64,
			.symbol_name = glyphmapping_name,
		},
	};

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, metadata, relocs);
	hostelf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, metadata_x8664, relocs_x8664);
}

void font_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct font_glyph {" << std::endl
		<< "	uint16_t width;" << std::endl
		<< "	uint16_t pixel_data_start_index;" << std::endl
		<< "};" << std::endl
		<< "struct font_byte_to_glyph_trie {" << std::endl
		<< "	uint8_t start;" << std::endl
		<< "	uint8_t end;" << std::endl
		<< "	uint16_t glyphs_start_index;" << std::endl
		<< "	uint16_t byte_to_glyph_index;" << std::endl
		<< "};" << std::endl
		<< "struct font {" << std::endl
		<< "	const uint16_t* pixel_data;" << std::endl
		<< "	const struct font_glyph* glyphs;" << std::endl
		<< "	const struct font_byte_to_glyph_trie* byte_to_glyph_trie;" << std::endl
		<< "	uint16_t glyph_height;" << std::endl
		<< "};" << std::endl;
}

const type_functions font_type_functions(
	  &font_write_struct
	, nullptr
	, nullptr
	, nullptr
	, &font_write_to_elf
);
