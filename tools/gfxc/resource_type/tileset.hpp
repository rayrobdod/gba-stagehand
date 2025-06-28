#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "image.hpp"
#include "object.hpp"

struct tileset {
	std::string var_name;
	std::array<rgba16_t, 16> palette;
	std::vector<uint8_t> tiles;

	explicit tileset(const std::pair<std::filesystem::path, bufferedimage>);
	void write(std::ostream& headerstream, Object& elf) const;

	static void write_struct(std::ostream& headerstream);
};
