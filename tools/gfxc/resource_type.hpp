#include "image.hpp"

enum type {
	TYPE_SPRITE,
	TYPE_TILESET,
	TYPE_TILESET_MONO,
	TYPE_SCENE,
	TYPE_SCENE_MODE3,
};

std::ostream& operator<<(std::ostream& os, enum type typ);

enum type resource_type(const bufferedimage&);
