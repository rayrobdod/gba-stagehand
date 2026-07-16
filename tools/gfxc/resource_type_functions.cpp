#include "resource_type_functions.hpp"

#include "resource_type/background.hpp"
#include "resource_type/background_horizontal_scroll.hpp"
#include "resource_type/background_mode3.hpp"
#include "resource_type/font.hpp"
#include "resource_type/isometric_object.hpp"
#include "resource_type/isometric_road.hpp"
#include "resource_type/sprite.hpp"
#include "resource_type/tile16x3map.hpp"
#include "resource_type/tileset.hpp"
#include "resource_type/tileset_monochrome.hpp"

alt_palette_data::alt_palette_data(uint16_t _tag, std::map<rgba16_t, rgba16_t> _mapping) : tag(_tag), mapping(_mapping) {}

palette_data::palette_data() {}

palette_data::palette_data(
	  uint16_t _tag
	, std::vector<std::vector<rgba16_t>> _colorss
	, std::map<const std::string, alt_palette_data> _alternates
) : tag(_tag), colorss(_colorss), alternates(_alternates) {}

palette_data::palette_data(
	  uint16_t _tag
	, palette_data_builder _builder
) : tag(_tag) {
	for (auto colors : _builder.colorss) {
		this->colorss.emplace_back(colors.begin(), colors.end());
	}
}

tiles_data::tiles_data() {}

tiles_data::tiles_data(uint16_t _tag, std::vector<gbatile_4bpp> _tiles) :
	tag(_tag), tiles(_tiles) {}

bg_tile_t::bg_tile_t() : tile(0), hflip(false), vflip(false), palette(0) {}
bg_tile_t::bg_tile_t(uint16_t tile, bool hflip, bool vflip, uint16_t palette) : tile(tile), hflip(hflip), vflip(vflip), palette(palette) {}

std::array<uint8_t, 2> bg_tile_t::to_bytes(void) const {
	std::array<uint8_t, 2> retval = {
		static_cast<uint8_t>(this->tile),
		static_cast<uint8_t>((this->tile >> 8) | (hflip ? 0x4 : 0) | (vflip ? 0x8 : 0) | (palette << 4)),
	};
	return retval;
}

uint16_t bg_tile_t::to_short(void) const {
	return static_cast<uint16_t>((this->tile) | (hflip ? 0x400 : 0) | (vflip ? 0x800: 0) | (palette << 12));
}

bool operator==(const bg_tile_t& lhs, const bg_tile_t& rhs) {
	return ((lhs.tile == rhs.tile) && (lhs.hflip == rhs.hflip) && (lhs.vflip == rhs.vflip) && (lhs.palette == rhs.palette));
}

std::vector<uint16_t> tile16x3::to_shorts(void) const {
	std::vector<uint16_t> retval = {this->behavior};
	for (unsigned layer = 0; layer < 3; ++layer)
	for (unsigned sub = 0; sub < 4; ++sub) {
		retval.push_back(this->tiles[layer][sub].to_short());
	}
	return retval;
}

bool operator==(const tile16x3& lhs, const tile16x3& rhs) {
	bool retval = lhs.behavior == rhs.behavior;

	for (unsigned layer = 0; layer < 3; ++layer)
	for (unsigned sub = 0; sub < 4; ++sub)
		retval = retval && lhs.tiles[layer][sub] == rhs.tiles[layer][sub];

	return retval;
}

tile16x3s_data::tile16x3s_data() {}
tile16x3s_data::tile16x3s_data(std::vector<tile16x3> _tile16x3s) : tile16x3s(_tile16x3s) {}

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
	// TYPE_SUBRESOURCE intentionally left blank
	{TYPE_SPRITE, sprite_type_functions},
	{TYPE_FONT, font_type_functions},
	{TYPE_TILESET, tileset_type_functions},
	{TYPE_TILESET_MONOCHROME, tileset_monochrome_type_functions},
	{TYPE_BACKGROUND, background_type_functions},
	{TYPE_BACKGROUND_MODE3, background_mode3_type_functions},
	{TYPE_BACKGROUND_HORIZONTAL_SCROLL, background_horizontal_scroll_type_functions},
	{TYPE_WALKAROUND_TILEMAP, tile16x3map_type_functions},
	{TYPE_ISOMETRIC_OBJECT, isometric_object_type_functions},
	{TYPE_ISOMETRIC_ROAD, isometric_road_type_functions},
};

const std::map<type, type_functions> type_functionss(type_functionss_initializer);
