#include "resource_type_functions.hpp"

#include "resource_type/background.hpp"
#include "resource_type/background_horizontal_scroll.hpp"
#include "resource_type/background_mode3.hpp"
#include "resource_type/font.hpp"
#include "resource_type/sprite.hpp"
#include "resource_type/tileset.hpp"
#include "resource_type/tileset_monochrome.hpp"

alt_palette_data::alt_palette_data(uint16_t _tag, std::map<rgba16_t, rgba16_t> _mapping) : tag(_tag), mapping(_mapping) {}

palette_data::palette_data() {}

palette_data::palette_data(
	  uint16_t _tag
	, std::vector<std::vector<rgba16_t>> _colorss
	, std::map<const std::string, alt_palette_data> _alternates
) : tag(_tag), colorss(_colorss), alternates(_alternates) {}

tiles_data::tiles_data() {}

tiles_data::tiles_data(uint16_t _tag, std::vector<gbatile_4bpp> _tiles) :
	tag(_tag), tiles(_tiles) {}

/* * * * * * * * * * * * * * * * * * */

palette_data_builder::palette_data_builder() {}

void palette_data_builder::merge(palette_data_builder& source, std::string_view palette_name) {
	this->colorss.merge(source.colorss);

	for (auto const& addend_alt : source.alternates) {
		std::string alt_name = addend_alt.first;
		std::map<rgba16_t, rgba16_t> addend_replacements = addend_alt.second;

		auto this_alternate_ptr = this->alternates.try_emplace(alt_name).first;

		for (auto const& addend_replacement : addend_replacements) {
			rgba16_t from = addend_replacement.first;
			rgba16_t to = addend_replacement.second;

			auto i = this_alternate_ptr->second.find(from);
			if (i == this_alternate_ptr->second.end()) {
				this_alternate_ptr->second.emplace(from, to);
			} else if (to == i->second) {
				// do nothing
			} else {
				std::string msg;
				msg += palette_name;
				msg += "_";
				msg += alt_name;
				msg += ": contradictory alternate palette mapping";
				throw std::logic_error(msg);
			}
		}
	}
}

void palette_data_builder::condense_colors() {
	std::vector<std::set<rgba16_t>> colorss2(this->colorss.begin(), this->colorss.end());
	std::sort(colorss2.begin(), colorss2.end(),
		[](std::set<rgba16_t> a, std::set<rgba16_t> b) {
			return a.size() > b.size();
		}
	);

	std::vector<std::set<rgba16_t>> colorss3;
	for (std::set<rgba16_t> colors : colorss2) {
		unsigned pal_i;
		for (pal_i = 0; pal_i < colorss3.size(); pal_i++) {
			std::set<rgba16_t> colors_copy(colors.begin(), colors.end());

			std::set<rgba16_t> new_colors(colorss3[pal_i].begin(), colorss3[pal_i].end());
			new_colors.merge(colors_copy);

			if (new_colors.size() <= 16) {
				colorss3[pal_i] = new_colors;
				break;
			}
		}
		if (pal_i >= colorss3.size()) {
			colorss3.push_back(colors);
		}
	}

	std::set<std::set<rgba16_t>> colorss4(colorss3.begin(), colorss3.end());
	this->colorss = colorss4;
}

/* * * * * * * * * * * * * * * * * * */

const std::initializer_list<std::pair<const type, type_functions>> type_functionss_initializer {
	{TYPE_SPRITE, sprite_type_functions},
	{TYPE_FONT, font_type_functions},
	{TYPE_TILESET, tileset_type_functions},
	{TYPE_TILESET_MONOCHROME, tileset_monochrome_type_functions},
	{TYPE_BACKGROUND, background_type_functions},
	{TYPE_BACKGROUND_MODE3, background_mode3_type_functions},
	{TYPE_BACKGROUND_HORIZONTAL_SCROLL, background_horizontal_scroll_type_functions},
};

const std::map<type, type_functions> type_functionss(type_functionss_initializer);
