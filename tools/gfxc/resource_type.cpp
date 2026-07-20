#include "resource_type.hpp"

std::ostream& operator<<(std::ostream& os, enum type typ) {
	switch (typ) {
	case TYPE_SUBRESOURCE:
		os << "SUBRESOURCE";
		break;
	case TYPE_SPRITE:
		os << "SPRITE";
		break;
	case TYPE_FONT:
		os << "FONT";
		break;
	case TYPE_TILESET:
		os << "TILESET";
		break;
	case TYPE_TILESET_MONOCHROME:
		os << "TILESET_MONO";
		break;
	case TYPE_BACKGROUND:
		os << "SCENE";
		break;
	case TYPE_BACKGROUND_MODE3:
		os << "SCENE_MODE3";
		break;
	case TYPE_BACKGROUND_HORIZONTAL_SCROLL:
		os << "SCENE_HORIZ_SCROLL";
		break;
	case TYPE_WALKAROUND_TILEMAP:
		os << "WALKAROUND_TILEMAP";
		break;
	}
	return os;
}

enum type resource_type(const bufferedimage& image) {
	auto explicit_typ = image.properties().find("Type");
	if (explicit_typ != image.properties().end()) {
		if (explicit_typ->second == "Subresource")
			return TYPE_SUBRESOURCE;
		if (explicit_typ->second == "Sprite")
			return TYPE_SPRITE;
		if (explicit_typ->second == "Font")
			return TYPE_FONT;
		if (explicit_typ->second == "Tileset")
			return TYPE_TILESET;
		if (explicit_typ->second == "Tileset Monochrome")
			return TYPE_TILESET_MONOCHROME;
		if (explicit_typ->second == "Background")
			return TYPE_BACKGROUND;
		if (explicit_typ->second == "Background Mode 3")
			return TYPE_BACKGROUND_MODE3;
		if (explicit_typ->second == "Background Horizontal Scroll")
			return TYPE_BACKGROUND_HORIZONTAL_SCROLL;

		std::string msg("Unknown explicit type: ");
		msg += explicit_typ->second;
		throw std::runtime_error(msg);
	}

	if (image.width() <= 64 && image.height() <= 64) {
		return TYPE_SPRITE;
	}
	if (image.width() == 32 * 8 && image.height() > 20 * 8) {
		return TYPE_BACKGROUND;
	}
	return TYPE_TILESET;
}

enum type resource_type(const input_tile16x3map& image) {
	auto explicit_typ = image.properties().find("Type");
	if (explicit_typ != image.properties().end()) {
		if (explicit_typ->second == "Subresource")
			return TYPE_SUBRESOURCE;
		if (explicit_typ->second == "Walkaround")
			return TYPE_WALKAROUND_TILEMAP;

		std::string msg("Unknown explicit type: ");
		msg += explicit_typ->second;
		throw std::runtime_error(msg);
	}
	return TYPE_WALKAROUND_TILEMAP;
}
