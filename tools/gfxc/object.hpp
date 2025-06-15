#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <vector>
#include <elf.h>

struct variable_template {
	std::string_view name;
	uint8_t binding;
};

struct relocation_template {
	Elf32_Addr offset;
	unsigned char type;
	const char* symbol_name;
};

class ObjectImpl;

class Object {
private:
	std::unique_ptr<ObjectImpl> impl;

	void push_bytes_section(
			const void* data, Elf32_Word size_in_bytes,
			variable_template variable);

	void push_relocation_section(
			std::string_view target_name,
			const struct relocation_template* rels, size_t rel_count);

public:
	Object(std::filesystem::path out_file_name);
	~Object();

	template<std::ranges::contiguous_range ES>
	void push_bytes_section(
			ES data,
			variable_template variable) {
		this->push_bytes_section(
			std::ranges::data(data),
			std::ranges::size(data) * sizeof(typename ES::value_type),
			variable);
	}

	template<std::ranges::contiguous_range ES>
	void push_relocation_section(
			std::string_view target_name,
			ES rels) {
		this->push_relocation_section(
			target_name,
			std::ranges::data(rels),
			std::ranges::size(rels));
	}
};

#endif
