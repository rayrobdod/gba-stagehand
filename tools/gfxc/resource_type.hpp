#ifndef RESOURCE_TYPE_HPP
#define RESOURCE_TYPE_HPP

#include "image.hpp"
#include "input_tilemap.hpp"

enum type {
	TYPE_SUBRESOURCE,
	TYPE_SPRITE,
	TYPE_FONT,
	TYPE_TILESET,
	TYPE_TILESET_MONOCHROME,
	TYPE_BACKGROUND,
	TYPE_BACKGROUND_MODE3,
	TYPE_BACKGROUND_HORIZONTAL_SCROLL,
	TYPE_WALKAROUND_TILEMAP,
};

std::ostream& operator<<(std::ostream& os, enum type typ);

enum type resource_type(const bufferedimage&);
enum type resource_type(const input_tile16x3map&);

#endif
