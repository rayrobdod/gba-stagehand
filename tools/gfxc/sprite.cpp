#include "sprite.hpp"

#include <stdexcept>

enum sprite_size sprite_size(unsigned width, unsigned height) {
	if (width == 8 && height == 8)
		return SIZE_8x8;
	if (width == 16 && height == 16)
		return SIZE_16x16;
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
	}
	return os;
}
