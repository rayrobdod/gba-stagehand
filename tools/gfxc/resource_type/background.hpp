#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "image.hpp"
#include "object.h"

struct bg_tile_t {
	uint16_t tile : 10;
	bool hflip : 1;
	bool vflip : 1;
	uint16_t palette : 4;

	bg_tile_t(uint16_t tile);
	std::array<uint8_t, 2> to_bytes(void);
};

struct background {
	std::string var_name;
	uint16_t paltag;
	std::vector<uint8_t> tileset;
	std::vector<bg_tile_t> tilemap;

	void write(std::ostream& headerstream, struct Object* elf) const;

	static void write_struct(std::ostream& headerstream);
};
