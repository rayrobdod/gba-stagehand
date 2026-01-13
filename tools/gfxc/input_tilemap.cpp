#include "input_tilemap.hpp"

input_tile16x3map::input_tile16x3map(
		size_t width,
		size_t height,
		const std::vector<input_tile16x3>& tiles,
		const std::map<std::string, std::string>& properties)
	: _width(width), _height(height), _tiles(tiles), _properties(properties) {
}

unsigned input_tile16x3map::width() const { return this->_width; }
unsigned input_tile16x3map::height() const  { return this->_height; }
const std::vector<input_tile16x3>& input_tile16x3map::tiles() const  { return this->_tiles; }
const std::map<std::string, std::string>& input_tile16x3map::properties() const { return this->_properties; }
const input_tile16x3& input_tile16x3map::tile(size_t x, size_t y) const {
	return this->_tiles[x + this->_width * y];
}

/* * * * * * * * * * * * * * * */

#include <libtiled/map.h>
#include <libtiled/mapreader.h>
#include "find_palette_superset.hpp"

#include <iostream>

static const unsigned MASK_5_BIT = 0x1F;
static const unsigned MASK_8_BIT = 0xFF;
static uint8_t scale_8bit_to_5bit(unsigned input) {
	return (input * MASK_5_BIT + MASK_8_BIT / 2) / MASK_8_BIT;
}
static rgba16_t scale_qcolor_to_5bit(QColor input) {
	rgba16_t retval(
		scale_8bit_to_5bit(input.red()),
		scale_8bit_to_5bit(input.green()),
		scale_8bit_to_5bit(input.blue()),
		input.alpha() == 255);
	return retval;
}

struct layer_id {unsigned index; QString name;};
static const std::initializer_list<layer_id> layer_ids {
	{2, "bottom"},
	{1, "middle"},
	{0, "top"},
};

input_tile16x3map tilemap_deserialize(
		const std::string& file_name) {
	Tiled::MapReader reader;

	const QString q_file_name = QString::fromStdString(file_name);
	const std::unique_ptr<Tiled::Map> map = reader.readMap(q_file_name);
	if (!map) {
		std::string msg;
		msg += file_name;
		msg += ": could not read tilemap file";
		throw std::runtime_error(msg);
	}
	if (16 != map->tileWidth()) {
		std::string msg;
		msg += file_name;
		msg += ": requires tileWidth of 16. Was ";
		msg += std::format("{}", map->tileWidth());
		throw std::runtime_error(msg);
	}
	if (16 != map->tileHeight()) {
		std::string msg;
		msg += file_name;
		msg += ": requires tileHeight of 16. Was ";
		msg += std::format("{}", map->tileHeight());
		throw std::runtime_error(msg);
	}

	std::vector<input_tile16x3> metatiles(map->width() * map->height());

	for (layer_id layer_id : layer_ids) {
		const Tiled::TileLayer* const layer = map->findLayer(layer_id.name, Tiled::Layer::TileLayerType)->asTileLayer();

		for (int y = 0; y < layer->height(); ++y)
		for (int x = 0; x < layer->width(); ++x)
		{
			unsigned position_flat = x + map->width() * y;

			Tiled::Cell cell = layer->cellAt(x, y);
			if (cell.tileId() >= 0) {
				QImage tile_image = cell.tile()->image().toImage();
				std::vector<rgba16_t> gba_pixels(16*16);
				for (unsigned y = 0; y < 16; ++y)
				for (unsigned x = 0; x < 16; ++x) {
					gba_pixels[x + 16 * y] = scale_qcolor_to_5bit(tile_image.pixelColor(x, y));
				}

				metatiles.at(position_flat).pixelss[layer_id.index] = gba_pixels;
			}
		}
	}

	std::map<std::string, std::string> text;
	for (auto it = map->properties().begin(); it != map->properties().end(); ++it) {
		text.emplace(it.key().toStdString(), it.value().toString().toStdString());
	}

	input_tile16x3map retval(
		map->width(),
		map->height(),
		metatiles,
		text);
	return retval;
}
