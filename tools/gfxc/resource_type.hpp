#ifndef RESOURCE_TYPE_HPP
#define RESOURCE_TYPE_HPP

#include "image.hpp"

enum type {
	TYPE_SPRITE,
	TYPE_FONT,
	TYPE_TILESET,
	TYPE_TILESET_MONOCHROME,
	TYPE_BACKGROUND,
	TYPE_BACKGROUND_MODE3,
	TYPE_BACKGROUND_HORIZONTAL_SCROLL,
};

std::ostream& operator<<(std::ostream& os, enum type typ);

enum type resource_type(const bufferedimage&);

#endif
