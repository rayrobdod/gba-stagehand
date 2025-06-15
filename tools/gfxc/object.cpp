#include "object.hpp"

#include <stdlib.h>
#include <string.h>


StringTableBuilder::StringTableBuilder(void) {
	this->_data.push_back('\0');
}

Elf32_Word StringTableBuilder::push(std::string_view value) {
	Elf32_Word retval = this->_data.size();
	for (auto i = value.begin(); i != value.end(); i++) {
		this->_data.push_back(*i);
	}
	this->_data.push_back('\0');
	return retval;
}

Elf32_Word StringTableBuilder::find(std::string_view value) const {
	const char* data = this->_data.data();

	for (Elf32_Word i = 0; i < this->_data.size(); i++) {
		if (0 == strcmp(value.data(), &(data[i]))) {
			return i;
		}
	}
	return 0;
}

Elf32_Word StringTableBuilder::find_or_push(std::string_view value) {
	Elf32_Word retval = find(value);
	if (0 == retval) {
		retval = push(value);
	}
	return retval;
}

const char* StringTableBuilder::operator[](Elf32_Word index) const {
	return this->_data.data() + index;
}

std::span<const std::byte> StringTableBuilder::to_bytes(void) const {
	std::span<const char> ch(this->_data);
	std::span<const std::byte> retval = std::as_bytes(ch);
	return retval;
}


Object::Object(std::filesystem::path out_file_name) :
	header({
		.e_ident = {
			[EI_MAG0] = ELFMAG0,
			[EI_MAG1] = ELFMAG1,
			[EI_MAG2] = ELFMAG2,
			[EI_MAG3] = ELFMAG3,
			[EI_CLASS] = ELFCLASS32,
			[EI_DATA] = ELFDATA2LSB,
			[EI_VERSION] = EV_CURRENT,
			[EI_OSABI] = ELFOSABI_ARM,
			// rest are zeros
		},
		.e_type = ET_REL,
		.e_machine = EM_ARM,
		.e_version = EV_CURRENT,
		//.e_shoff = ???,
		.e_flags = 0x0,
		.e_ehsize = sizeof(Elf32_Ehdr),
		.e_shentsize = sizeof(Elf32_Shdr),
		//.e_shnum = ???,
		//.e_shstrndx = ???,
	}),
	section_strings(),
	symbol_strings(),
	private_symbols(),
	public_symbols(),
	relocation_sections(),
	sections()
{
	this->f = fopen(out_file_name.c_str(), "w");
	if (!f) {
		fprintf(stderr, "Could not open file %s\n", out_file_name.c_str());
		exit(1);
	}
	fseek(f, sizeof(Elf32_Ehdr), SEEK_SET);

	this->private_symbols.emplace_back();
	this->sections.emplace_back();
}

Object::~Object(void) {
	Elf32_Word predicted_symbols_index = sections.size() + relocation_sections.size() + 1;
	for (auto i = relocation_sections.begin(); i != relocation_sections.end(); i++) {
		std::string section_name(".rel");
		section_name.append(section_strings[sections[i->target].sh_name]);

		this->push_entries_section(
			((Elf32_Shdr_Template) {
				.sh_name = section_name,
				.sh_type = SHT_REL,
				.sh_link = predicted_symbols_index,
				.sh_info = i->target,
			}),
			i->data
		);
	}

	Elf32_Word strtab_index = sections.size();
	this->push_bytes_section(
		{
			.sh_name = ".strtab",
			.sh_type = SHT_STRTAB,
			.sh_addralign = 1,
		},
		symbol_strings.to_bytes()
	);

	std::vector<Elf32_Sym> all_symbols;
	all_symbols.reserve(private_symbols.size() + public_symbols.size());
	for (auto i = private_symbols.begin(); i != private_symbols.end(); i++) {
		all_symbols.push_back(*i);
	}
	for (auto i = public_symbols.begin(); i != public_symbols.end(); i++) {
		all_symbols.push_back(*i);
	}

	this->push_entries_section(
		{
			.sh_name = ".symtab",
			.sh_type = SHT_SYMTAB,
			.sh_link = strtab_index,
			.sh_info = static_cast<Elf32_Word>(private_symbols.size()),
			.sh_addralign = 4,
		},
		all_symbols
	);

	header.e_shstrndx = sections.size();
	// `push` first to make sure there is no problem with mutation order
	section_strings.push(".shstrtab");
	this->push_bytes_section(
		{
			.sh_name = ".shstrtab",
			.sh_type = SHT_STRTAB,
			.sh_addralign = 1,
		},
		section_strings.to_bytes()
	);
	header.e_shoff = static_cast<Elf32_Word>(ftell(f));
	header.e_shnum = sections.size();

	fwrite(sections.data(), sizeof(Elf32_Shdr), sections.size(), f);

	rewind(f);
	fwrite(&header, sizeof(Elf32_Ehdr), 1, f);

	fclose(this->f);
}

void Object::push_bytes_section(
		const Elf32_Shdr_Template header,
		const std::span<const std::byte> data) {

	Elf32_Off offset = static_cast<Elf32_Word>(ftell(this->f));
	fwrite(data.data(), sizeof(std::byte), data.size(), f);
	sections.push_back({
		.sh_name = this->section_strings.find_or_push(header.sh_name),
		.sh_type = header.sh_type,
		.sh_flags = header.sh_flags,
		.sh_addr = header.sh_addr,
		.sh_offset = offset,
		.sh_size = static_cast<Elf32_Word>(data.size()),
		.sh_link = header.sh_link,
		.sh_info = header.sh_info,
		.sh_addralign = header.sh_addralign,
		.sh_entsize = 0,
	});

}

Elf32_Section Object::index_of_section(const char* name) const {
	Elf32_Section i = 0;
	for (auto s = sections.begin(); s != sections.end(); s++, i++) {
		if (0 == strcmp(section_strings[s->sh_name], name)) {
			return i;
		}
	}
	return 0;
}

uint32_t Object::id_of_symbol(const char* name) const {
	uint32_t i = 0;
	for (auto s = private_symbols.begin(); s != private_symbols.end(); s++, i++) {
		if (0 == strcmp(symbol_strings[s->st_name], name)) {
			return i;
		}
	}
	for (auto s = public_symbols.begin(); s != public_symbols.end(); s++, i++) {
		if (0 == strcmp(symbol_strings[s->st_name], name)) {
			return i;
		}
	}
	return 0;
}
