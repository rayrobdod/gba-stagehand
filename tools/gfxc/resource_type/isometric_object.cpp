#include "resource_type/isometric_object.hpp"

#include <iostream>
#include <stdexcept>
#include "find_palette_superset.hpp"
#include "object.hpp"
#include "struct_bytes_builder.hpp"

static void isometric_object_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct isometric_object {" << std::endl
		<< "	uint16_t size;" << std::endl
		<< "	bg_tile_t tiles[];" << std::endl
		<< "};" << std::endl;
}

struct halftile_position {uint8_t x; uint8_t y; bool points_right;};
static const std::initializer_list<halftile_position> halftile_positions_1 = {
	{0, 8, false},
	{8, 8, true},
	{0, 4, true},
	{8, 4, false},
	{0, 0, false},
	{8, 0, true},
};

class isohalftile_mask_image : public image {
private:
	const image* const _backing;
	const bool _points_right;

public:
	explicit isohalftile_mask_image(const image* backing, bool points_right) : _backing(backing), _points_right(points_right) {}

	unsigned width() const override { return this->_backing->width(); }
	unsigned height() const override { return this->_backing->height(); }
	rgba16_t pixel(unsigned x, unsigned y) const override {
		unsigned maybe_flipped_x = (this->_points_right ? x : this->width() - x - 1);
		unsigned symmetric_y = (y < this->height() / 2 ? y : this->height() - y - 1);
		if (maybe_flipped_x <= symmetric_y * 2) {
			return this->_backing->pixel(x, y);
		} else {
			return rgba16_t::TRANSPARENT;
		}
	}
};

class image_isometric_halftile_iterator {
private:
	const bufferedimage* const _backing;
	unsigned _index;

public:
	using difference_type = std::ptrdiff_t;
	using value_type = bufferedimage;

	explicit image_isometric_halftile_iterator(const bufferedimage* backing, unsigned index) : _backing(backing), _index(index) {}
	image_isometric_halftile_iterator(const image_isometric_halftile_iterator& other) : _backing(other._backing), _index(other._index) {}

	bool operator==(const image_isometric_halftile_iterator& other) const {
		return this->_backing == other._backing && this->_index == other._index;
	}
	bool operator!=(const image_isometric_halftile_iterator& other) const  {
		return ! (*this == other);
	}
	image_isometric_halftile_iterator& operator++() {
		++this->_index;
		return *this;
	}
	image_isometric_halftile_iterator operator++(int) {
		image_isometric_halftile_iterator retval(*this);
		this->operator++();
		return retval;
	}
	bufferedimage operator*() const {
		halftile_position pos = halftile_positions_1.begin()[_index];

		subimage sub = this->_backing->sub(pos.x, pos.y, 8, 8);
		isohalftile_mask_image masked(&sub, pos.points_right);

		std::vector<rgba16_t> pixels(masked.pixels().begin(), masked.pixels().end());

		bufferedimage retval(8, 8, pixels, {}, _backing->background(), {}, {});
		return retval;
	}
};

class image_isometric_halftile_range {
private:
	const bufferedimage* const _backing;

public:
	explicit image_isometric_halftile_range(const bufferedimage* backing) : _backing(backing) {}
	image_isometric_halftile_iterator begin() const {
		image_isometric_halftile_iterator retval(this->_backing, 0);
		return retval;
	}
	image_isometric_halftile_iterator end() const {
		image_isometric_halftile_iterator retval(this->_backing, 6);
		return retval;
	}
};


static palette_data_builder isometric_object_extract_palettes(input_path_and_data input) {
	palette_data_builder retval;

	bufferedimage image = std::get<bufferedimage>(input.second);
	rgba16_t background = image.background().with_alpha(0);
	std::set<std::set<rgba16_t>> tile_palettes;
	image_isometric_halftile_range range(&image);
	for (auto subimg : range) {
		std::set<rgba16_t> new_pal;
		new_pal.insert(background);
		new_pal.merge(subimg.palette());
		if (new_pal.size() > 16) {
			std::string msg(input.first.string());
			msg += ": tile palette larger than 16 colors";
			throw std::logic_error(msg);
		}
		tile_palettes.insert(new_pal);
	}

	for (auto pal : tile_palettes) {
		retval.colorss.insert(pal);
	}

	return retval;
}

static std::vector<gbatile_4bpp> isometric_object_extract_tiles(input_path_and_data input, palette_data palettes) {
	std::vector<gbatile_4bpp> retval;
	bufferedimage image = std::get<bufferedimage>(input.second);

	image_isometric_halftile_range range(&image);
	for (auto subimg : range) {
		uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
		const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

		const gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));
		const gbatile_4bpp_matcher tile1m(tile1);

		if (retval.end() == std::find_if(retval.begin(), retval.end(), tile1m)) {
			retval.push_back(tile1);
		}
	}

	return retval;
}

std::vector<bg_tile_t> isometric_object_extract_map(
	input_path_and_data input,
	palette_data palettes,
	tiles_data tiles_dat
) {
	std::vector<bg_tile_t> retval;
	bufferedimage image = std::get<bufferedimage>(input.second);

	const std::vector<gbatile_4bpp> tiles(tiles_dat.tiles);

	image_isometric_halftile_range range(&image);
	for (auto subimg : range) {
		uint16_t pal_i = find_palette_superset<std::vector<std::vector<rgba16_t>>>(palettes.colorss, subimg.palette());
		const std::vector<rgba16_t> used_pal = palettes.colorss[pal_i];

		const gbatile_4bpp tile1(subimg.to_tile_4bpp(used_pal));
		gbatile_4bpp tile1h(tile1);
		tile1h.hflip();
		gbatile_4bpp tile1v(tile1);
		tile1v.vflip();
		gbatile_4bpp tile1hv(tile1h);
		tile1hv.vflip();

		unsigned tile_i;
		for (tile_i = 0; tile_i < tiles.size(); tile_i++) {
			if (tile1 == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, false, false, pal_i));
				break;
			}
			if (tile1h == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, true, false, pal_i));
				break;
			}
			if (tile1v == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, false, true, pal_i));
				break;
			}
			if (tile1hv == tiles[tile_i]) {
				retval.push_back(bg_tile_t(tile_i, true, true, pal_i));
				break;
			}
		}
		if (tile_i >= tiles.size()) {
			std::string msg(input.first);
			msg += ": lost tile between `background_extract_tiles` and `background_write_to_elf`";
			throw std::logic_error(msg);
		}
	}

	return retval;
}

static void isometric_object_write_to_elf(
	input_path_and_data input,
	std::pair<std::string, palette_data> palettes,
	std::pair<std::string, tiles_data> tiles,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const struct isometric_object " << var_name << ";" << std::endl;

	std::vector<bg_tile_t> tilemap = isometric_object_extract_map(input, palettes.second, tiles.second);

	StructBytesBuilder serialized;
	serialized.push_uint16(1);
	for (bg_tile_t entry : tilemap) {
		serialized.push_uint16(entry.to_short());
	}

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized.bytes_arm, serialized.relocs_arm);
	hostelf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, serialized.bytes_x8664, serialized.relocs_x8664);
}

const type_functions isometric_object_type_functions(
	  &isometric_object_write_struct
	, &isometric_object_extract_palettes
	, &isometric_object_extract_tiles
	, nullptr
	, &isometric_object_write_to_elf
);
