#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <iostream>
#include <map>
#include <ostream>
#include <set>
#include <string>
#include <vector>

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
} rgba16_t;

bool operator<(const rgba16_t&, const rgba16_t&);
bool operator==(const rgba16_t&, const rgba16_t&);

static const rgba16_t TRANSPARENT_BLACK = {0, 0, 0, 0};
static const rgba16_t TRANSPARENT_MAGENTA = {31, 0, 31, 0};

std::ostream& operator<<(std::ostream& os, const rgba16_t& value);

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
};

class image_pixel_iterator : public std::iterator<std::forward_iterator_tag, rgba16_t> {
private:
	const image* const _backing;
	unsigned _x;
	unsigned _y;

public:
	explicit image_pixel_iterator(const image*, unsigned x, unsigned y);

	bool operator==(const image_pixel_iterator&) const;
	bool operator!=(const image_pixel_iterator&) const;
	void operator++();
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

class image_tile_iterator : public std::iterator<std::forward_iterator_tag, rgba16_t> {
private:
	const image* const _backing;
	const unsigned _width;
	const unsigned _height;
	unsigned _left;
	unsigned _top;

public:
	explicit image_tile_iterator(const image*, unsigned width, unsigned height, unsigned left, unsigned top);

	bool operator==(const image_tile_iterator&) const;
	bool operator!=(const image_tile_iterator&) const;
	void operator++();
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

	unsigned width() const;
	unsigned height() const;
	rgba16_t pixel(unsigned x, unsigned y) const;
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

	unsigned width() const;
	unsigned height() const;
	rgba16_t pixel(unsigned x, unsigned y) const;

	const std::map<std::string, std::string>& text() const;
	rgb15_t background() const;

	std::map<std::string, std::vector<rgba16_t>> alt_palettes(std::vector<rgba16_t> palette) const;
};

#endif //  #ifndef IMAGE_H
