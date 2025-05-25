#include <cstdint>
#include <string>
#include <vector>

struct bg_tile_t {
	uint16_t tile : 10;
	bool hflip : 1;
	bool vflip : 1;
	uint16_t palette : 4;

	bg_tile_t(uint16_t tile);
};

struct background {
	std::string var_name;
	uint16_t paltag;
	std::vector<uint8_t> tileset;
	std::vector<bg_tile_t> tilemap;
};
