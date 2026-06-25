#include "struct_bytes_builder.hpp"

StructBytesBuilder::StructBytesBuilder() : alignment_arm(1), alignment_x8664(1) {}

void StructBytesBuilder::balign(unsigned alignment) {
	alignment_arm = std::max(alignment_arm, alignment);
	alignment_x8664 = std::max(alignment_x8664, alignment);
	while (bytes_arm.size() % alignment != 0)
		bytes_arm.push_back(0);
	while (bytes_x8664.size() % alignment != 0)
		bytes_x8664.push_back(0);
}

void StructBytesBuilder::pointer_align() {
	alignment_arm = std::max(alignment_arm, 4u);
	alignment_x8664 = std::max(alignment_x8664, 8u);
	while (bytes_arm.size() % 4 != 0)
		bytes_arm.push_back(0);
	while (bytes_x8664.size() % 8 != 0)
		bytes_x8664.push_back(0);
}

void StructBytesBuilder::self_align() {
	while (bytes_arm.size() % alignment_arm != 0)
		bytes_arm.push_back(0);
	while (bytes_x8664.size() % alignment_x8664 != 0)
		bytes_x8664.push_back(0);
}

void StructBytesBuilder::push_uint16(uint16_t v) {
	this->balign(2);
	for (unsigned i = 0; i < 16; i += 8) {
		bytes_arm.push_back(static_cast<uint8_t>(v >> i));
		bytes_x8664.push_back(static_cast<uint8_t>(v >> i));
	}
}

void StructBytesBuilder::push_uint32(uint32_t v) {
	this->balign(4);
	for (unsigned i = 0; i < 32; i += 8) {
		bytes_arm.push_back(static_cast<uint8_t>(v >> i));
		bytes_x8664.push_back(static_cast<uint8_t>(v >> i));
	}
}

void StructBytesBuilder::push_pointer(std::string_view symbol_name) {
	this->push_pointer(symbol_name, 0);
}

void StructBytesBuilder::push_pointer(std::string_view symbol_name, int offset) {
	this->pointer_align();
	relocs_arm.push_back({
		.offset = static_cast<Elf32_Addr>(bytes_arm.size()),
		.type = R_ARM_ABS32,
		.symbol_name = static_cast<std::string>(symbol_name),
	});
	relocs_x8664.push_back({
		.offset = static_cast<Elf64_Addr>(bytes_x8664.size()),
		.type = R_X86_64_64,
		.symbol_name = static_cast<std::string>(symbol_name),
	});
	this->push_uint32(offset);
	for (unsigned i = 0; i < 32; i += 8) {
		bytes_x8664.push_back(0);
	}
}

void StructBytesBuilder::push_uint16_relocated(std::string_view symbol_name) {
	this->push_uint16_relocated(symbol_name, 0);
}

void StructBytesBuilder::push_uint16_relocated(std::string_view symbol_name, int offset) {
	this->balign(2);
	relocs_arm.push_back({
		.offset = static_cast<Elf32_Addr>(bytes_arm.size()),
		.type = R_ARM_ABS16,
		.symbol_name = static_cast<std::string>(symbol_name),
	});
	relocs_x8664.push_back({
		.offset = static_cast<Elf64_Addr>(bytes_x8664.size()),
		.type = R_X86_64_16,
		.symbol_name = static_cast<std::string>(symbol_name),
	});

	for (unsigned i = 0; i < 16; i += 8) {
		bytes_arm.push_back(static_cast<uint8_t>(offset >> i));
		bytes_x8664.push_back(static_cast<uint8_t>(offset >> i));
	}
}

void StructBytesBuilder::push_tileset(
	const std::pair<std::string, palette_data> palettes,
	const std::pair<std::string, tiles_data> tiles
) {
	this->push_pointer(palettes.first);
	this->push_uint16(static_cast<uint16_t>(palettes.second.colorss.size()));
	this->push_uint16(palettes.second.tag);
	this->push_pointer(tiles.first);
	this->push_uint16(static_cast<uint16_t>(tiles.second.tiles.size()));
	this->push_uint16(tiles.second.tag);
}
