#include "image.hpp"

#include <cstdlib>
#include <iomanip>

rgb15_t rgba16_t::strip_alpha() const {
	return {this->r, this->g, this->b};
}
rgba16_t rgb15_t::with_alpha(uint16_t a) const {
	return {this->r, this->g, this->b, a != 0};
}
const rgba16 rgba16_t::BLACK = {0, 0, 0, 1};
const rgb15 rgb15_t::BLACK = {0, 0, 0};

bool operator==(const rgba16_t& lhs, const rgba16_t& rhs) {
	return (lhs.a == 0 && rhs.a == 0) || ((lhs.a == rhs.a) && (lhs.r == rhs.r) && (lhs.g == rhs.g) && (lhs.b == rhs.b));
}

bool operator<(const rgba16_t& lhs, const rgba16_t& rhs) {
	if (lhs.a == 0 && rhs.a == 0)
		return false;
	if (lhs.a != rhs.a)
		return lhs.a < rhs.a;
	if (lhs.r != rhs.r)
		return lhs.r < rhs.r;
	if (lhs.g != rhs.g)
		return lhs.g < rhs.g;
	return lhs.b < rhs.b;
}

std::array<uint8_t, 2> rgba16_t::to_bytes(void) {
	uint16_t as_int = reinterpret_cast<uint16_t*>(this)[0];
	std::array<uint8_t, 2> retval = {
		static_cast<uint8_t>(as_int),
		static_cast<uint8_t>(as_int >> 8),
	};
	return retval;
}

std::ostream& operator<<(std::ostream& os, const rgba16_t& value) {
	return os << "rgba16(" << value.r << "," << value.g << "," << value.b << "," << value.a << ")";
}

////

gbatile_4bpp::gbatile_4bpp(const std::vector<uint8_t>& bytes) : _bytes(bytes) {}

void gbatile_4bpp::hflip() {
	if (this->_bytes.size() != 32) {
		std::string msg("can only flip 8x8 tiles");
		throw std::logic_error(msg);
	}

	for (int y = 0; y < 8; y++) {
		uint32_t _a = this->_bytes[y * 4];
		_a |= (this->_bytes[y * 4 + 1] << 8);
		_a |= (this->_bytes[y * 4 + 2] << 16);
		_a |= (this->_bytes[y * 4 + 3] << 24);
		uint32_t _b = (_a & 0xF0F0F0F0) >> 4;
		_b |= (_a & 0x0F0F0F0F) << 4;

		this->_bytes[y * 4] = _b >> 24;
		this->_bytes[y * 4 + 1] = _b >> 16;
		this->_bytes[y * 4 + 2] = _b >> 8;
		this->_bytes[y * 4 + 3] = _b;
	}
}

void gbatile_4bpp::vflip() {
	if (this->_bytes.size() != 32) {
		std::string msg("can only flip 8x8 tiles");
		throw std::logic_error(msg);
	}

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			std::swap(this->_bytes[y * 4 + x], this->_bytes[(7 - y) * 4 + x]);
		}
	}
}

std::vector<uint8_t> gbatile_4bpp::bytes() const {return this->_bytes;}

bool gbatile_4bpp::operator==(const gbatile_4bpp& other) const {return this->_bytes == other._bytes;}

std::ostream& operator<<(std::ostream& os, const gbatile_4bpp& value) {
	os << "gbatile_4bpp[";
	for (int i = 0; i < 32; i++) {
		os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value.bytes()[i]) << " ";
	}
	os << "]";
	return os;
}


gbatile_4bpp_matcher::gbatile_4bpp_matcher(const gbatile_4bpp& _tile) : tile(_tile) {}

bool gbatile_4bpp_matcher::operator()(gbatile_4bpp other) const {
	if (this->tile == other)
		return true;

	other.hflip();
	if (this->tile == other)
		return true;

	other.vflip();
	if (this->tile == other)
		return true;

	other.hflip();
	if (this->tile == other)
		return true;

	return false;
}

////

image_pixel_range image::pixels() const {
	return image_pixel_range(this);
}

subimage image::sub(unsigned left, unsigned top, unsigned width, unsigned height) const {
	return subimage(this, left, top, width, height);
}

image_tile_range image::subs(unsigned width, unsigned height) const {
	return image_tile_range(this, width, height);
}

std::set<rgba16_t> image::palette() const {
	std::set<rgba16_t> retval(this->pixels().begin(), this->pixels().end());
	return retval;
}

////

bufferedimage::bufferedimage(
	unsigned width,
	unsigned height,
	std::vector<rgba16_t> pixels,
	std::map<std::string, std::string> text,
	rgb15_t background,
	std::map<std::string, std::map<rgba16_t, rgba16_t>> alt_palettes)
		: _width(width),
			_height(height),
			_pixels(pixels),
			_text(text),
			_background(background),
			_alt_palettes(alt_palettes) {
}

unsigned bufferedimage::width() const {
	return this->_width;
}

unsigned bufferedimage::height() const {
	return this->_height;
}

const std::map<std::string, std::string>& bufferedimage::text() const {
	return this->_text;
}

rgba16_t bufferedimage::pixel(unsigned x, unsigned y) const {
	return this->_pixels[y * this->_width + x];
}

rgb15_t bufferedimage::background() const {
	return this->_background;
}

std::map<std::string, std::vector<rgba16_t>> bufferedimage::alt_palettes(std::vector<rgba16_t> palette) const {
	std::map<std::string, std::vector<rgba16_t>> retval;
	for (auto alt : this->_alt_palettes) {
		std::string name(alt.first);
		std::map<rgba16_t, rgba16_t> mapping(alt.second);
		std::vector<rgba16_t> new_pal;

		for (auto color : palette) {
			auto x = mapping.find(color);
			if (x == mapping.end()) {
				new_pal.push_back(color);
			} else {
				new_pal.push_back(x->second);
			}
		}
		retval.insert(make_pair(name, new_pal));
	}
	return retval;
}

std::map<std::string, std::map<rgba16_t, rgba16_t>> bufferedimage::alt_palettes() const {
	return this->_alt_palettes;
}

////

subimage::subimage(const image* backing, unsigned left, unsigned top, unsigned width, unsigned height)
		: _backing(backing), _left(left), _top(top), _width(width), _height(height) {
}

unsigned subimage::width() const {
	return this->_width;
}

unsigned subimage::height() const {
	return this->_height;
}

rgba16_t subimage::pixel(unsigned x, unsigned y) const {
	return this->_backing->pixel(this->_left + x, this->_top + y);
}

////

image_pixel_range::image_pixel_range(const image* backing)
		: _backing(backing) {
}

image_pixel_iterator image_pixel_range::begin() const {
	return image_pixel_iterator(_backing, 0, 0);
}

image_pixel_iterator image_pixel_range::end() const {
	if (_backing->width() == 0) {
		return image_pixel_iterator(_backing, 0, 0);
	} else {
		return image_pixel_iterator(_backing, 0, _backing->height());
	}
}

////

image_pixel_iterator::image_pixel_iterator(const image* backing, unsigned x, unsigned y)
		: _backing(backing), _x(x), _y(y) {
}

image_pixel_iterator::image_pixel_iterator(const image_pixel_iterator& other)
		: _backing(other._backing), _x(other._x), _y(other._y) {
}

bool image_pixel_iterator::operator==(const image_pixel_iterator& other) const {
	return this->_backing == other._backing && this->_x == other._x && this->_y == other._y;
}

bool image_pixel_iterator::operator!=(const image_pixel_iterator& other) const {
	return ! (*this == other);
}

image_pixel_iterator& image_pixel_iterator::operator++() {
	++this->_x;
	if (this->_x >= this->_backing->width()) {
		this->_x = 0;
		this->_y++;
	}
	return *this;
}

image_pixel_iterator image_pixel_iterator::operator++(int) {
	image_pixel_iterator retval(*this);
	this->operator++();
	return retval;
}

rgba16_t image_pixel_iterator::operator*() const {
	return this->_backing->pixel(this->_x, this->_y);
}

////

image_tile_range::image_tile_range(const image* backing, unsigned width, unsigned height)
		: _backing(backing), _width(width), _height(height) {
}

image_tile_iterator image_tile_range::begin() const {
	return image_tile_iterator(_backing, _width, _height, 0, 0);
}

image_tile_iterator image_tile_range::end() const {
	return image_tile_iterator(_backing, _width, _height, 0, _backing->height() / _height);
}

////

image_tile_iterator::image_tile_iterator(const image* backing, unsigned width, unsigned height, unsigned left, unsigned top)
		: _backing(backing), _width(width), _height(height), _left(left), _top(top) {
}

image_tile_iterator::image_tile_iterator(const image_tile_iterator& other)
		: _backing(other._backing), _width(other._width), _height(other._height), _left(other._left), _top(other._top) {
}

bool image_tile_iterator::operator==(const image_tile_iterator& other) const {
	return this->_backing == other._backing &&
		this->_width == other._width &&
		this->_height == other._height &&
		this->_left == other._left &&
		this->_top == other._top;
}

bool image_tile_iterator::operator!=(const image_tile_iterator& other) const {
	return ! (*this == other);
}

image_tile_iterator& image_tile_iterator::operator++() {
	++this->_left;
	if (this->_left >= this->_backing->width() / this->_width) {
		this->_left = 0;
		this->_top++;
	}
	return *this;
}

image_tile_iterator image_tile_iterator::operator++(int) {
	image_tile_iterator retval(*this);
	this->operator++();
	return retval;
}

subimage image_tile_iterator::operator*() const {
	return this->_backing->sub(this->_left * this->_width, this->_top * this->_height, this->_width, this->_height);
}
