#include "object_x8664.hpp"

Object_x8664::Object_x8664(std::filesystem::path out_file_name) :
	header({
		.e_ident = {
			[EI_MAG0] = ELFMAG0,
			[EI_MAG1] = ELFMAG1,
			[EI_MAG2] = ELFMAG2,
			[EI_MAG3] = ELFMAG3,
			[EI_CLASS] = ELFCLASS64,
			[EI_DATA] = ELFDATA2LSB,
			[EI_VERSION] = EV_CURRENT,
			[EI_OSABI] = ELFOSABI_NONE,
			// rest are zeros
		},
		.e_type = ET_REL,
		.e_machine = EM_X86_64,
		.e_version = EV_CURRENT,
		//.e_shoff = ???,
		.e_flags = 0x0,
		.e_ehsize = sizeof(Elf64_Ehdr),
		.e_shentsize = sizeof(Elf64_Shdr),
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
	fseek(f, sizeof(Elf64_Ehdr), SEEK_SET);

	this->private_symbols.emplace_back();
	this->sections.emplace_back();

	std::ranges::empty_view<const std::byte> empty;
	this->push_bytes_section(
		{
			.sh_name = ".note.GNU-stack",
			.sh_type = SHT_NOTE,
		},
		empty
	);

}

Object_x8664::~Object_x8664(void) {
	Elf64_Word predicted_symbols_index = sections.size() + relocation_sections.size() + 1;
	for (auto i = relocation_sections.begin(); i != relocation_sections.end(); i++) {
		std::string section_name(".rel");
		section_name.append(section_strings[sections[i->target].sh_name]);

		std::vector<Elf64_Rel> rel_data;

		for (auto rel : i->data) {
			uint32_t symbol = id_of_symbol(rel.symbol_name);

			rel_data.push_back({
				.r_offset = rel.offset,
				.r_info = ELF64_R_INFO(symbol, rel.type),
			});
		}

		this->push_entries_section(
			((Elf64_Shdr_Template) {
				.sh_name = section_name,
				.sh_type = SHT_REL,
				.sh_link = predicted_symbols_index,
				.sh_info = i->target,
			}),
			rel_data
		);
	}

	Elf64_Word strtab_index = sections.size();
	this->push_bytes_section(
		{
			.sh_name = ".strtab",
			.sh_type = SHT_STRTAB,
			.sh_addralign = 1,
		},
		symbol_strings.to_bytes()
	);

	std::vector<Elf64_Sym> all_symbols;
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
			.sh_info = static_cast<Elf64_Word>(private_symbols.size()),
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
	header.e_shoff = static_cast<Elf64_Word>(ftell(f));
	header.e_shnum = sections.size();

	fwrite(sections.data(), sizeof(Elf64_Shdr), sections.size(), f);

	rewind(f);
	fwrite(&header, sizeof(Elf64_Ehdr), 1, f);

	fclose(this->f);
}

void Object_x8664::push_bytes_section(
		const Elf64_Shdr_Template header,
		const std::span<const std::byte> data) {

	Elf64_Off offset = static_cast<Elf64_Word>(ftell(this->f));
	fwrite(data.data(), sizeof(std::byte), data.size(), f);
	sections.push_back({
		.sh_name = this->section_strings.find_or_push(header.sh_name),
		.sh_type = header.sh_type,
		.sh_flags = header.sh_flags,
		.sh_addr = header.sh_addr,
		.sh_offset = offset,
		.sh_size = static_cast<Elf64_Word>(data.size()),
		.sh_link = header.sh_link,
		.sh_info = header.sh_info,
		.sh_addralign = header.sh_addralign,
		.sh_entsize = 0,
	});

}

void Object_x8664::push_symbol(Elf64_Sym_Template hdr) {

	uint32_t st_name = symbol_strings.push(hdr.name);
	Elf64_Section shndx = index_of_section(hdr.section);
	if (0 == shndx) {
		fprintf(stderr, "symbol %s pushed before section %s\n", hdr.name.c_str(), hdr.section.c_str());
		exit(1);
	}
	uint16_t st_shndx = static_cast<uint16_t>(shndx);

	(hdr.binding == STB_LOCAL ? private_symbols : public_symbols).push_back({
		.st_name = st_name,
		.st_info = ELF64_ST_INFO(hdr.binding, hdr.type),
		.st_other = hdr.st_other,
		.st_shndx = st_shndx,
		.st_value = hdr.st_value,
		.st_size = hdr.st_size,
	});
}

Elf64_Section Object_x8664::index_of_section(const std::string_view name) const {
	if (name == "<ABS>") return SHN_ABS;

	Elf64_Section i = 0;
	for (auto s = sections.begin(); s != sections.end(); s++, i++) {
		if (name == section_strings[s->sh_name]) {
			return i;
		}
	}
	return 0;
}

uint32_t Object_x8664::id_of_symbol(const std::string_view name) const {
	uint32_t i = 0;
	for (auto s = private_symbols.begin(); s != private_symbols.end(); s++, i++) {
		if (name == symbol_strings[s->st_name]) {
			return i;
		}
	}
	for (auto s = public_symbols.begin(); s != public_symbols.end(); s++, i++) {
		if (name == symbol_strings[s->st_name]) {
			return i;
		}
	}
	return 0;
}
