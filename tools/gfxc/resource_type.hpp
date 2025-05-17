#include "image.hpp"

enum type {
	TYPE_SPRITE,
	TYPE_FONT,
	TYPE_TILESET,
	TYPE_TILESET_MONOCHROME,
	TYPE_BACKGROUND,
	TYPE_BACKGROUND_MODE3,
};

std::ostream& operator<<(std::ostream& os, enum type typ);

enum type resource_type(const bufferedimage&);
