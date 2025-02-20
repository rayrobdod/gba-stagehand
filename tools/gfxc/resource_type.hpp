#include "image.hpp"

enum type {
	TYPE_SPRITE,
	TYPE_TILESET,
	TYPE_SCENE,
};

std::ostream& operator<<(std::ostream& os, enum type typ);

enum type resource_type(const bufferedimage&);
