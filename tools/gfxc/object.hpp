#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <filesystem>
#include <memory>
#include <ranges>
#include <string>
#include <vector>
#include <elf.h>

/** Elf32_Shdr but without the offset or size */
typedef struct {
	std::string	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Word	sh_addr;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
} Elf32_Shdr_Template;

struct variable_template {
	std::string_view name;
	uint8_t binding;
};

struct relocation_template {
	Elf32_Addr offset;
	unsigned char type;
	const char* symbol_name;
};

struct Relocations {
	uint32_t target;
	std::vector<Elf32_Rel> data;
};


class StringTableBuilder {
private:
	std::vector<char> _data;
public:
	StringTableBuilder(void);

	/** inserts string into the buffer; returns position of inserted string */
	Elf32_Word push(std::string_view);

	/** returns position of string, or `0` if not found */
	Elf32_Word find(std::string_view) const;

	/** inserts string into the buffer if it is not already there; returns position of string */
	Elf32_Word find_or_push(std::string_view);

	const char* operator[](Elf32_Word index) const;

	std::span<const std::byte> to_bytes(void) const;
};

template<std::ranges::contiguous_range DATAS>
static Elf32_Word size_in_bytes(DATAS data) {
	return sizeof(typename DATAS::value_type) * std::ranges::size(data);
}

class Object {
private:
	FILE* f;
	Elf32_Ehdr header;
	StringTableBuilder section_strings;
	StringTableBuilder symbol_strings;
	std::vector<Elf32_Sym> private_symbols;
	std::vector<Elf32_Sym> public_symbols;
	std::vector<Relocations> relocation_sections;
	std::vector<Elf32_Shdr> sections;

	Elf32_Section index_of_section(const char* name) const;
	uint32_t id_of_symbol(const char* name) const;

public:
	Object(std::filesystem::path out_file_name);
	~Object();

	void push_bytes_section(
		const Elf32_Shdr_Template header,
		const std::span<const std::byte> data);

	template<std::ranges::contiguous_range DATAS>
	void push_bytes_section(
			const Elf32_Shdr_Template header,
			DATAS data) {
		std::span<typename DATAS::value_type, std::dynamic_extent> a(data);
		std::span<const std::byte> b = std::as_bytes(a);
		this->push_bytes_section(header, b);
	}

	template<std::ranges::contiguous_range DATAS>
	void push_entries_section(
			const Elf32_Shdr_Template header,
			const DATAS data) {

		Elf32_Off offset = (Elf32_Off) ftell(this->f);
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

	template<std::ranges::contiguous_range DATAS>
	void push_single_variable_rodata_sections(
			variable_template variable,
			DATAS data) {
		std::array<relocation_template, 0> rels;
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

		Elf32_Section data_section_index = (Elf32_Section) sections.size();

		this->push_bytes_section(
			{
				.sh_name = data_section_name,
				.sh_type = SHT_PROGBITS,
				.sh_flags = SHF_ALLOC,
				.sh_addralign = 4,
			},
			data
		);

		private_symbols.push_back({
			.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION),
			.st_shndx = data_section_index,
		});
		(variable.binding == STB_LOCAL ? private_symbols : public_symbols).push_back({
			.st_name = symbol_strings.push(variable.name),
			.st_value = 0x00,
			.st_size = size_in_bytes(data),
			.st_info = ELF32_ST_INFO(variable.binding, STT_OBJECT),
			.st_shndx = data_section_index,
		});

		if (! std::ranges::empty(rels)) {
			std::vector<Elf32_Rel> rel_data;

			for (auto rel : rels) {
				uint32_t symbol = id_of_symbol(rel.symbol_name);

				rel_data.push_back({
					.r_offset = rel.offset,
					.r_info = ELF32_R_INFO(symbol, rel.type),
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
