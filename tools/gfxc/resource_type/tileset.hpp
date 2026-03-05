#include "resource_type_functions.hpp"
#include <span>
#include "object.hpp"

palette_data_builder tileset_extract_palettes(input_path_and_data);

void tileset_serialized(
	std::span<uint16_t, 8> bytebuffer,
	std::span<relocation_template, 2> relocs,
	std::span<uint16_t, 16> bytebuffer_x8664,
	std::span<relocation_template_x8664, 2> relocs_x8664,
	const std::pair<std::string, palette_data> palettes,
	const std::pair<std::string, tiles_data> tiles);

extern const type_functions tileset_type_functions;
