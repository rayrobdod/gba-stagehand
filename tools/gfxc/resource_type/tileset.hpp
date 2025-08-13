#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "image.hpp"
#include "object.hpp"
#include "resource_type_functions.hpp"

struct tileset {
	std::string var_name;
	std::array<rgba16_t, 16> palette;
	std::vector<uint8_t> tiles;

	explicit tileset(const std::pair<std::filesystem::path, bufferedimage>);
	void write(std::ostream& headerstream, Object& elf) const;
};

palette_data_builder tileset_extract_palettes(std::pair<std::filesystem::path, bufferedimage> in);

extern const type_functions tileset_type_functions;
