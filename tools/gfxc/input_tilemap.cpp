#include "input_tilemap.hpp"

input_tile16x3map::input_tile16x3map(
		size_t width,
		size_t height,
		const std::vector<input_tile16x3>& tiles,
		const std::vector<input_tile16x3map::sign>& signs,
		const std::vector<input_tile16x3map::warp>& warps,
		const std::map<std::string, std::string>& properties)
	:
		_width(width),
		_height(height),
		_tiles(tiles),
		_signs(signs),
		_warps(warps),
		_properties(properties)
	{
}

unsigned input_tile16x3map::width() const { return this->_width; }
unsigned input_tile16x3map::height() const  { return this->_height; }
const std::vector<input_tile16x3>& input_tile16x3map::tiles() const  { return this->_tiles; }
const std::vector<input_tile16x3map::sign>& input_tile16x3map::signs() const { return this->_signs; }
const std::vector<input_tile16x3map::warp>& input_tile16x3map::warps() const { return this->_warps; }
const std::map<std::string, std::string>& input_tile16x3map::properties() const { return this->_properties; }
const input_tile16x3& input_tile16x3map::tile(size_t x, size_t y) const {
	return this->_tiles[x + this->_width * y];
}

/* * * * * * * * * * * * * * * */

#include <libtiled/map.h>
#include <libtiled/mapobject.h>
#include <libtiled/mapreader.h>
#include <libtiled/objectgroup.h>
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

const std::initializer_list<std::string_view> walkaround_behavior_names = {
	"WB_NORMAL",
	"WB_IMPASSABLE",
	"WB_IMPASSABLE_N",
	"WB_IMPASSABLE_E",
	"WB_IMPASSABLE_W",
	"WB_IMPASSABLE_S",
	"WB_IMPASSABLE_NE",
	"WB_IMPASSABLE_NW",
	"WB_IMPASSABLE_NS",
	"WB_IMPASSABLE_SE",
	"WB_IMPASSABLE_SW",
	"WB_IMPASSABLE_EW",
};

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

				if (layer_id.name == "middle") {
					for (auto prop = cell.tile()->properties().begin(); prop != cell.tile()->properties().end(); ++prop) {
						if (prop.key().toStdString() == "Behavior") {
							std::string target_name = prop.value().toString().toStdString();

							auto wb = walkaround_behavior_names.begin();
							for (; wb != walkaround_behavior_names.end(); ++wb) {
								if (target_name == *wb) {
									metatiles.at(position_flat).behavior_id = wb - walkaround_behavior_names.begin();
									break;
								}
							}
							if (wb == walkaround_behavior_names.end()) {
								std::cerr <<
										file_name <<
										": Unknown walkaround_behavior: " <<
										target_name <<
										std::endl;
							}

							break;
						}
					}
				}
			}
		}
	}

	std::vector<input_tile16x3map::sign> signs;
	std::vector<input_tile16x3map::warp> warps;
	{
		const Tiled::ObjectGroup* const layer = map->findLayer("events", Tiled::Layer::ObjectGroupType)->asObjectGroup();

		for (auto it = layer->objects().begin(); it != layer->objects().end(); ++it) {
			if ("Sign" == (*it)->effectiveType()) {
				signs.emplace_back(
					(*it)->x() / 16,
					((*it)->y() - (*it)->height()) / 16,
					(*it)->resolvedProperty("message").toString().toStdString()
				);
			}
			else if ("Warp" == (*it)->effectiveType()) {
				warps.emplace_back(
					(*it)->name().toStdString(),
					(*it)->x() / 16,
					((*it)->y() - (*it)->height()) / 16,
					(*it)->resolvedProperty("destination_map").toString().toStdString(),
					(*it)->resolvedProperty("destination_warp").toString().toStdString()
				);
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
		signs,
		warps,
		text);
	return retval;
}
