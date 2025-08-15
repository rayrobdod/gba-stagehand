#include "resource_type/background_mode3.hpp"

#include "object.hpp"
#include "variable_name_for_image.hpp"

static void mode3_write_to_elf(
	std::pair<std::filesystem::path, struct bufferedimage> image,
	[[gnu::unused]] std::pair<std::string, palette_data> palettes,
	[[gnu::unused]] std::pair<std::string, tiles_data> tiles,
	std::string var_name,
	std::ostream& headerstream,
	Object& elf
) {
	headerstream << "extern const rgb_t " << var_name << "[160][240];" << std::endl;

	if (image.second.width() != 240 || image.second.height() != 160) {
		std::string msg(image.first.string());
		msg += ": mode3 background does not have expected dimensions";
		throw std::logic_error(msg);
	}

	std::vector<rgba16_t> imgdata(image.second.pixels().begin(), image.second.pixels().end());

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, imgdata);
}

const type_functions background_mode3_type_functions(
	  nullptr
	, nullptr
	, nullptr
	, &mode3_write_to_elf
);
