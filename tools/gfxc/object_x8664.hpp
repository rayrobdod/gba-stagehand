#ifndef OBJECT_x8664_HPP
#define OBJECT_x8664_HPP

#include <filesystem>
#include <ranges>
#include <string>
#include <vector>
#include <elf.h>
#include "object.hpp"

/** Elf64_Shdr but without the offset or size */
typedef struct {
	std::string	sh_name;
	Elf64_Word	sh_type;
	Elf64_Word	sh_flags;
	Elf64_Word	sh_addr;
	Elf64_Word	sh_link;
	Elf64_Word	sh_info;
	Elf64_Word	sh_addralign;
} Elf64_Shdr_Template;

typedef struct {
	std::string	name;
	Elf64_Addr	st_value;
	uint32_t	st_size;
	unsigned	binding;
	unsigned	type;
	unsigned char	st_other;
	std::string	section;
} Elf64_Sym_Template;

struct relocation_template_x8664 {
	Elf64_Addr offset;
	unsigned char type;
	std::string symbol_name;
};

struct Relocations_x8664 {
	uint32_t target;
	std::vector<Elf64_Rel> data;
};


class Object_x8664 {
private:
	FILE* f;
	Elf64_Ehdr header;
	StringTableBuilder section_strings;
	StringTableBuilder symbol_strings;
	std::vector<Elf64_Sym> private_symbols;
	std::vector<Elf64_Sym> public_symbols;
	std::vector<Relocations_x8664> relocation_sections;
	std::vector<Elf64_Shdr> sections;

	Elf64_Section index_of_section(const std::string_view name) const;
	uint32_t id_of_symbol(const std::string_view name) const;

public:
	Object_x8664(std::filesystem::path out_file_name);
	~Object_x8664();

	void push_bytes_section(
		const Elf64_Shdr_Template header,
		const std::span<const std::byte> data);

	void push_symbol(Elf64_Sym_Template);

	template<std::ranges::contiguous_range DATAS>
	void push_bytes_section(
			const Elf64_Shdr_Template header,
			DATAS data) {
		std::span<typename DATAS::value_type, std::dynamic_extent> a(data);
		std::span<const std::byte> b = std::as_bytes(a);
		this->push_bytes_section(header, b);
	}

	template<std::ranges::contiguous_range DATAS>
	void push_entries_section(
			const Elf64_Shdr_Template header,
			const DATAS data) {

		Elf64_Off offset = (Elf64_Off) ftell(this->f);
		fwrite(std::ranges::data(data), sizeof(typename DATAS::value_type), std::ranges::size(data), f);
		sections.push_back({
			.sh_name = this->section_strings.find_or_push(header.sh_name),
			.sh_type = header.sh_type,
			.sh_flags = header.sh_flags,
			.sh_addr = header.sh_addr,
			.sh_offset = offset,
			.sh_size = size_in_bytes(data),
			.sh_link = header.sh_link,
			.sh_info = header.sh_info,
			.sh_addralign = header.sh_addralign,
			.sh_entsize = sizeof(typename DATAS::value_type),
		});
	}

	template<std::ranges::contiguous_range RELS>
	void push_relocation_section(
			std::string_view data_section_name,
			RELS rels) {
		std::string rel_section_name(".rel");
		rel_section_name += data_section_name;

		std::vector<Elf64_Rel> rel_data;

		Elf64_Section data_section_index = index_of_section(data_section_name);

		for (auto rel : rels) {
			uint32_t symbol = id_of_symbol(rel.symbol_name);

			rel_data.push_back({
				.r_offset = rel.offset,
				.r_info = ELF64_R_INFO(symbol, rel.type),
			});
		}

		relocation_sections.push_back({
			.target = data_section_index,
			.data = rel_data,
		});
	}

	template<std::ranges::contiguous_range DATAS>
	void push_single_variable_rodata_sections(
			variable_template variable,
			DATAS data) {
		std::array<relocation_template_x8664, 0> rels;
		this->push_single_variable_rodata_sections(
			variable,
			data,
			rels
		);
	}

	template<std::ranges::contiguous_range DATAS, std::ranges::contiguous_range RELS>
	void push_single_variable_rodata_sections(
			variable_template variable,
			DATAS data,
			RELS rels) {
		std::string data_section_name(".rodata.");
		data_section_name += variable.name;
		std::string rel_section_name(".rel");
		rel_section_name += data_section_name;

		if (! std::ranges::empty(rels))
			section_strings.push(rel_section_name);

		Elf64_Section data_section_index = (Elf64_Section) sections.size();

		this->push_bytes_section(
			{
				.sh_name = data_section_name,
				.sh_type = SHT_PROGBITS,
				.sh_flags = SHF_ALLOC,
				.sh_addralign = 4,
			},
			data
		);

		if (false) {
		private_symbols.push_back({
			.st_info = ELF64_ST_INFO(STB_LOCAL, STT_SECTION),
			.st_shndx = data_section_index,
		});
		}
		(variable.binding == STB_LOCAL ? private_symbols : public_symbols).push_back({
			.st_name = symbol_strings.push(variable.name),
			.st_info = ELF64_ST_INFO(variable.binding, STT_OBJECT),
			.st_shndx = data_section_index,
			.st_value = 0x00,
			.st_size = size_in_bytes(data),
		});

		if (! std::ranges::empty(rels)) {
			std::vector<Elf64_Rel> rel_data;

			for (auto rel : rels) {
				uint32_t symbol = id_of_symbol(rel.symbol_name);

				rel_data.push_back({
					.r_offset = rel.offset,
					.r_info = ELF64_R_INFO(symbol, rel.type),
				});
			}

			relocation_sections.push_back({
				.target = data_section_index,
				.data = rel_data,
			});
		}
	}
};

#endif
