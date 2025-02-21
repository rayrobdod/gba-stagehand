#include "sprite.hpp"

#include <stdexcept>

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
