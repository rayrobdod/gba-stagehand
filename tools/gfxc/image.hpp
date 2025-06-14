#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <iterator>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>
#include <ranges>

struct rgba16;

typedef struct {
	uint16_t r : 5;
	uint16_t g : 5;
	uint16_t b : 5;

	struct rgba16 with_alpha(uint16_t a) const;
} rgb15_t;

typedef struct rgba16 {
	uint16_t r : 5;
	uint16_t g : 5;
	uint16_t b : 5;
	uint16_t a : 1;

	rgb15_t strip_alpha() const;

	static const rgba16 BLACK;
} rgba16_t;

bool operator<(const rgba16_t&, const rgba16_t&);
bool operator==(const rgba16_t&, const rgba16_t&);

std::ostream& operator<<(std::ostream& os, const rgba16_t& value);

//

class gbatile_4bpp {
	std::vector<uint8_t> _bytes;
public:
	gbatile_4bpp(std::vector<uint8_t> bytes);

	std::vector<uint8_t> bytes() const;

	bool operator==(const gbatile_4bpp&) const;
};

//

class image_pixel_range;
class image_tile_range;
class subimage;

class image {
public:
	virtual unsigned width() const = 0;
	virtual unsigned height() const = 0;
	virtual rgba16_t pixel(unsigned x, unsigned y) const = 0;

	image_pixel_range pixels() const;
	subimage sub(unsigned left, unsigned top, unsigned width, unsigned height) const;
	image_tile_range subs(unsigned width, unsigned height) const;

	std::set<rgba16_t> palette() const;

	template<std::ranges::range R>
	gbatile_4bpp to_tile_4bpp(R palette) const;
};

class image_pixel_iterator {
private:
	const image* _backing;
	unsigned _x;
	unsigned _y;

public:
	using difference_type = std::ptrdiff_t;
	using value_type = rgba16_t;

	explicit image_pixel_iterator(const image*, unsigned x, unsigned y);
	image_pixel_iterator(const image_pixel_iterator&);

	bool operator==(const image_pixel_iterator&) const;
	bool operator!=(const image_pixel_iterator&) const;
	image_pixel_iterator& operator++();
	image_pixel_iterator operator++(int);
	rgba16_t operator*() const;
};

class image_pixel_range {
private:
	const image* const _backing;

public:
	explicit image_pixel_range(const image*);
	image_pixel_iterator begin() const;
	image_pixel_iterator end() const;
};

class image_tile_iterator {
private:
	const image* const _backing;
	const unsigned _width;
	const unsigned _height;
	unsigned _left;
	unsigned _top;

public:
	using difference_type = std::ptrdiff_t;
	using value_type = subimage;

	explicit image_tile_iterator(const image*, unsigned width, unsigned height, unsigned left, unsigned top);
	image_tile_iterator(const image_tile_iterator&);

	bool operator==(const image_tile_iterator&) const;
	bool operator!=(const image_tile_iterator&) const;
	image_tile_iterator& operator++();
	image_tile_iterator operator++(int);
	subimage operator*() const;
};

class image_tile_range {
private:
	const image* const _backing;
	const unsigned _width;
	const unsigned _height;

public:
	explicit image_tile_range(const image*, unsigned width, unsigned height);
	image_tile_iterator begin() const;
	image_tile_iterator end() const;
};

class subimage : public image {
private:
	const image* const _backing;
	const unsigned _left;
	const unsigned _top;
	const unsigned _width;
	const unsigned _height;

public:
	subimage(const image*, unsigned left, unsigned top, unsigned width, unsigned height);

	unsigned width() const override;
	unsigned height() const override;
	rgba16_t pixel(unsigned x, unsigned y) const override;
};

class bufferedimage : public image {
private:
	const unsigned _width;
	const unsigned _height;
	const std::vector<rgba16_t> _pixels;
	const std::map<std::string, std::string> _text;
	const rgb15_t _background;
	const std::map<std::string, std::map<rgba16_t, rgba16_t>> _alt_palettes;

public:
	bufferedimage(
		unsigned width,
		unsigned height,
		std::vector<rgba16_t> pixels,
		std::map<std::string, std::string> text,
		rgb15_t background,
		std::map<std::string, std::map<rgba16_t, rgba16_t>> alt_palettes);

	unsigned width() const override;
	unsigned height() const override;
	rgba16_t pixel(unsigned x, unsigned y) const override;

	const std::map<std::string, std::string>& text() const;
	rgb15_t background() const;

	std::map<std::string, std::vector<rgba16_t>> alt_palettes(std::vector<rgba16_t> palette) const;
};

#include <algorithm>
#include "subword_output_iterator.hpp"

template<std::ranges::range R>
gbatile_4bpp image::to_tile_4bpp(R palette) const {
	subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> builder;
	for (auto pixel : this->pixels()) {
		auto palptr = std::find(palette.begin(), palette.end(), pixel);
		uint4_t palindex(palptr - palette.begin());

		*builder = palindex;
		++builder;
	}
	gbatile_4bpp retval(builder.result());
	return retval;
}

#endif //  #ifndef IMAGE_H
