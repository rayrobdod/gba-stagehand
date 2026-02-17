#include "resource_type/background_mode3.hpp"

#include "object.hpp"

static void mode3_write_to_elf(
	input_path_and_data input,
	[[gnu::unused]] std::pair<std::string, palette_data> palettes,
	[[gnu::unused]] std::pair<std::string, tiles_data> tiles,
	[[maybe_unused]] std::pair<std::string, tile16x3s_data> tile16x3s,
	std::string var_name,
	std::ostream& headerstream,
	[[maybe_unused]] Object_x8664& hostelf,
	Object& elf
) {
	headerstream << "extern const rgb_t " << var_name << "[160][240];" << std::endl;

	bufferedimage image = std::get<bufferedimage>(input.second);

	if (image.width() != 240 || image.height() != 160) {
		std::string msg(input.first.string());
		msg += ": mode3 background does not have expected dimensions";
		throw std::logic_error(msg);
	}

	std::vector<rgba16_t> imgdata(image.pixels().begin(), image.pixels().end());

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, imgdata);
}

const type_functions background_mode3_type_functions(
	  nullptr
	, nullptr
	, nullptr
	, nullptr
	, &mode3_write_to_elf
);
