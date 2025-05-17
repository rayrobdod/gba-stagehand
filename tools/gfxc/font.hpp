#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include "image.hpp"
#include "object.h"

struct font_glyph {
	uint8_t width;
	std::vector<uint8_t> data;
};

struct font {
	std::string var_name;
	uint8_t height;
	std::vector<font_glyph> glyphs;

	explicit font(const std::pair<std::filesystem::path, bufferedimage>);
	void write(std::ostream& headerstream, struct Object* elf) const;

	static void write_struct(std::ostream& headerstream);
};
