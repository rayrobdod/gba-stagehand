#include "resource_type/tileset_monochrome.hpp"

#include "object.hpp"

static void tileset_monochrome_write_struct(std::ostream& headerstream) {
	headerstream << std::endl
		<< "struct bitpacked_tileset {" << std::endl
		<< "	uint16_t size;" << std::endl
		<< "	uint16_t unit_width;" << std::endl
		<< "	char data[];" << std::endl
		<< "};" << std::endl;
}

static void tileset_monochrome_write_to_elf(
	std::pair<std::filesystem::path, struct bufferedimage> image,
	[[gnu::unused]] std::pair<std::string, palette_data> palettes,
	[[gnu::unused]] std::pair<std::string, tiles_data> tiles,
	std::string var_name,
	std::ostream& headerstream,
	Object& elf
) {
	headerstream << "extern const struct bitpacked_tileset " << var_name << ";" << std::endl;

	subword_output_iterator<uint8_t, uint1_t, DIRECTION_INC> tiledata_builder;
	for (auto subimg : image.second.subs(8, 8)) {
		for (auto pixel : subimg.pixels()) {
			uint1_t palindex(pixel == rgba16_t::BLACK ? 1 : 0);

			*tiledata_builder = palindex;
			++tiledata_builder;
		}
	}
	std::vector tiledata = tiledata_builder.result();
	unsigned size = tiledata.size();
	tiledata.insert(tiledata.begin(), 0);
	tiledata.insert(tiledata.begin(), 1);
	tiledata.insert(tiledata.begin(), (sizeof(uint8_t) * size) >> 8);
	tiledata.insert(tiledata.begin(), sizeof(uint8_t) * size);

	elf.push_single_variable_rodata_sections({var_name, STB_GLOBAL}, tiledata);
}

const type_functions tileset_monochrome_type_functions(
	  &tileset_monochrome_write_struct
	, nullptr
	, nullptr
	, &tileset_monochrome_write_to_elf
);
