#ifndef OBJECTS_SECTION_BUILDER_HPP
#define OBJECTS_SECTION_BUILDER_HPP

#include <vector>
#include "object.hpp"
#include "object_x8664.hpp"
#include "resource_type_functions.hpp"

class StructBytesBuilder {
	unsigned alignment_arm;
	unsigned alignment_x8664;
public:
	std::vector<uint8_t> bytes_arm;
	std::vector<relocation_template> relocs_arm;
	std::vector<uint8_t> bytes_x8664;
	std::vector<relocation_template_x8664> relocs_x8664;

	StructBytesBuilder();

	void balign(unsigned alignment);
	void pointer_align();
	void self_align();

	void push_uint16(uint16_t v);

	void push_uint32(uint32_t v);

	void push_pointer(std::string_view symbol_name);
	void push_pointer(std::string_view symbol_name, int offset);

	void push_uint16_relocated(std::string_view symbol_name);
	void push_uint16_relocated(std::string_view symbol_name, int offset);

	void push_tileset(
		const std::pair<std::string, palette_data> palettes,
		const std::pair<std::string, tiles_data> tiles
	);
};

#endif        //  #ifndef OBJECTS_SECTION_BUILDER_HPP
