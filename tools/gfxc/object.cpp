#include "object.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <elf.h>

/** Elf32_Shdr but without the offset or size */
typedef struct {
	Elf32_Word	sh_name;
	Elf32_Word	sh_type;
	Elf32_Word	sh_flags;
	Elf32_Word	sh_addr;
	Elf32_Word	sh_link;
	Elf32_Word	sh_info;
	Elf32_Word	sh_addralign;
} Elf32_Shdr_Template;


class StringTableBuilder {
private:
	std::vector<char> _data;
public:
	StringTableBuilder(void) {
		this->_data.push_back('\0');
	}

	/** inserts string into the buffer if it is not already there; returns position of string */
	Elf32_Word push(const char* value) {
		Elf32_Word retval = this->_data.size();
		while ('\0' != *value) {
			this->_data.push_back(*value);
			++value;
		}
		this->_data.push_back('\0');
		return retval;
	}

	Elf32_Word push(std::string value) {
		Elf32_Word retval = this->_data.size();
		for (auto i = value.begin(); i != value.end(); i++) {
			this->_data.push_back(*i);
		}
		this->_data.push_back('\0');
		return retval;
	}

	Elf32_Word find(const char* value) const {
		const char* data = this->data();

		for (Elf32_Word i = 0; i < this->size(); i++) {
			if (0 == strcmp(value, &(data[i]))) {
				return i;
			}
		}
		return 0;
	}

	Elf32_Word find_or_push(const char* value) {
		Elf32_Word retval = find(value);
		if (0 == retval) {
			retval = push(value);
		}
		return retval;
	}

	Elf32_Word size(void) const {
		return this->_data.size();
	}

	const char* data(void) const {
		return this->_data.data();
	}
};


struct Relocations {
	uint32_t target;
	std::vector<Elf32_Rel> data;
};


static Elf32_Shdr writeBytesSection(
		const Elf32_Shdr_Template header,
		const void* data,
		const Elf32_Word size,
		FILE* f
	) {
	Elf32_Off offset = (Elf32_Off) ftell(f);

	fwrite(data, sizeof(char), size, f);

	return {
		.sh_name = header.sh_name,
		.sh_type = header.sh_type,
		.sh_flags = header.sh_flags,
		.sh_addr = header.sh_addr,
		.sh_offset = offset,
		.sh_size = size,
		.sh_link = header.sh_link,
		.sh_info = header.sh_info,
		.sh_addralign = header.sh_addralign,
		.sh_entsize = 0,
	};
}

static Elf32_Shdr writeEntrySection(
		const Elf32_Shdr_Template header,
		const void* data,
		const Elf32_Word entsize,
		const Elf32_Word nmemb,
		FILE* f
	) {
	Elf32_Off offset = (Elf32_Off) ftell(f);

	fwrite(data, entsize, nmemb, f);

	return {
		.sh_name = header.sh_name,
		.sh_type = header.sh_type,
		.sh_flags = header.sh_flags,
		.sh_addr = header.sh_addr,
		.sh_offset = offset,
		.sh_size = entsize * nmemb,
		.sh_link = header.sh_link,
		.sh_info = header.sh_info,
		.sh_addralign = header.sh_addralign,
		.sh_entsize = entsize,
	};
}

static Elf32_Shdr writeFileCopySection(
		const Elf32_Shdr_Template header,
		const char* from_filename,
		FILE* f
	) {
	Elf32_Off offset = (Elf32_Off) ftell(f);

	FILE* from_file = fopen(from_filename, "r");
	if (NULL == from_file) {
		fprintf(stderr, "Could not open file %s\n", from_filename);
		exit(1);
	}
	const size_t buffer_size = 0x400;
	char* buffer = (char*) malloc(buffer_size);

	while (!feof(from_file) && !ferror(from_file)) {
		size_t readSize = fread(buffer, 1, buffer_size, from_file);
		size_t writeSize = fwrite(buffer, 1, readSize, f);

		if (readSize != writeSize) {
			free(buffer);
			fclose(from_file);
			fprintf(stderr, "Incomplete file write %s\n", from_filename);
			exit(1);
		}
	}

	int ferrorValue = ferror(from_file);

	free(buffer);
	fclose(from_file);

	if (ferrorValue) {
		fprintf(stderr, "Error (%d) reading file %s", ferrorValue, from_filename);
		exit(1);
	}

	if (0 != ftell(f) % 4) {
		fwrite("\0\0\0\0", 1, 4 - (ftell(f) % 4), f);
	}

	return {
		.sh_name = header.sh_name,
		.sh_type = header.sh_type,
		.sh_flags = header.sh_flags,
		.sh_addr = header.sh_addr,
		.sh_offset = offset,
		.sh_size = ((Elf32_Word) ftell(f)) - offset,
		.sh_link = header.sh_link,
		.sh_info = header.sh_info,
		.sh_addralign = header.sh_addralign,
		.sh_entsize = 0,
	};
}


class ObjectImpl {
private:
	FILE* f;
	Elf32_Ehdr header;
	StringTableBuilder section_strings;
	StringTableBuilder symbol_strings;
	std::vector<Elf32_Sym> private_symbols;
	std::vector<Elf32_Sym> public_symbols;
	std::vector<Relocations> relocation_sections;
	std::vector<Elf32_Shdr> sections;
public:
	ObjectImpl(const char* out_file_name) :
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
		this->f = fopen(out_file_name, "w");
		if (!f) {
			fprintf(stderr, "Could not open file %s\n", out_file_name);
			exit(1);
		}
		fseek(f, sizeof(Elf32_Ehdr), SEEK_SET);

		this->private_symbols.emplace_back();
		this->sections.emplace_back();
	}

	~ObjectImpl(void) {
		Elf32_Word predicted_symbols_index = sections.size() + relocation_sections.size() + 1;
		for (auto i = relocation_sections.begin(); i != relocation_sections.end(); i++) {
			std::string section_name(".rel");
			section_name.append(&section_strings.data()[sections[i->target].sh_name]);

			sections.push_back(writeEntrySection(
				((Elf32_Shdr_Template) {
					.sh_name = section_strings.push(section_name),
					.sh_type = SHT_REL,
					.sh_link = predicted_symbols_index,
					.sh_info = i->target,
				}),
				i->data.data(),
				sizeof(Elf32_Rel),
				i->data.size(),
				this->f
			));
		}

		Elf32_Word strtab_index = sections.size();
		sections.push_back(writeBytesSection(
			{
				.sh_name = section_strings.push(".strtab"),
				.sh_type = SHT_STRTAB,
				.sh_addralign = 1,
			},
			symbol_strings.data(),
			symbol_strings.size(),
			f
		));

		std::vector<Elf32_Sym> all_symbols;
		all_symbols.reserve(private_symbols.size() + public_symbols.size());
		for (auto i = private_symbols.begin(); i != private_symbols.end(); i++) {
			all_symbols.push_back(*i);
		}
		for (auto i = public_symbols.begin(); i != public_symbols.end(); i++) {
			all_symbols.push_back(*i);
		}

		sections.push_back(writeEntrySection(
			{
				.sh_name = section_strings.push(".symtab"),
				.sh_type = SHT_SYMTAB,
				.sh_link = strtab_index,
				.sh_info = (Elf32_Word) private_symbols.size(),
				.sh_addralign = 4,
			},
			all_symbols.data(),
			sizeof(Elf32_Sym),
			all_symbols.size(),
			f
		));

		header.e_shstrndx = sections.size();
		section_strings.push(".shstrtab");
		sections.push_back(writeBytesSection(
			{
				// `find` to make sure there is no problem with mutation order
				.sh_name = section_strings.find(".shstrtab"),
				.sh_type = SHT_STRTAB,
				.sh_addralign = 1,
			},
			section_strings.data(),
			section_strings.size(),
			f
		));
		header.e_shoff = (Elf32_Word) ftell(f);
		header.e_shnum = sections.size();

		fwrite(sections.data(), sizeof(Elf32_Shdr), sections.size(), f);

		rewind(f);
		fwrite(&header, sizeof(Elf32_Ehdr), 1, f);

		fclose(this->f);
	}

	void push_file_copy_section(
		const char* src_filename,
		const variable_template variable
	) {
		std::string section_name(".rodata.");
		section_name.append(variable.name);

		Elf32_Section my_section_index = (Elf32_Section) sections.size();

		Elf32_Shdr my_section_header = writeFileCopySection(
			{
				.sh_name = section_strings.push(section_name),
				.sh_type = SHT_PROGBITS,
				.sh_flags = SHF_ALLOC,
				.sh_addralign = 4,
			},
			src_filename,
			this->f
		);

		sections.push_back(my_section_header);

		private_symbols.push_back({
			.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION),
			.st_shndx = my_section_index,
		});
		(variable.binding == STB_LOCAL ? private_symbols : public_symbols).push_back({
			.st_name = symbol_strings.push(variable.name),
			.st_value = 0x00,
			.st_size = my_section_header.sh_size,
			.st_info = ELF32_ST_INFO(variable.binding, STT_OBJECT),
			.st_shndx = my_section_index,
		});
	}

	void push_bytes_section(
		const void* data, Elf32_Word size,
		const variable_template variable
	) {
		std::string section_name(".rodata.");
		section_name.append(variable.name);

		Elf32_Section my_section_index = (Elf32_Section) sections.size();

		Elf32_Shdr my_section_header = writeBytesSection(
			{
				.sh_name = section_strings.push(section_name),
				.sh_type = SHT_PROGBITS,
				.sh_flags = SHF_ALLOC,
				.sh_addralign = 4,
			},
			data, size,
			this->f
		);

		sections.push_back(my_section_header);

		private_symbols.push_back({
			.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION),
			.st_shndx = my_section_index,
		});
		(variable.binding == STB_LOCAL ? private_symbols : public_symbols).push_back({
			.st_name = symbol_strings.push(variable.name),
			.st_value = 0x00,
			.st_size = my_section_header.sh_size,
			.st_info = ELF32_ST_INFO(variable.binding, STT_OBJECT),
			.st_shndx = my_section_index,
		});
	}

	void push_relocation_section(
		const char* variable_name,
		const struct relocation_template* rels, size_t rel_count
	) {
		std::string target_name(".rodata.");
		target_name += variable_name;
		Elf32_Section target = index_of_section(target_name.c_str());
		std::vector<Elf32_Rel> data;

		for (size_t i = 0; i < rel_count; i++) {
			uint32_t symbol = id_of_symbol(rels[i].symbol_name);

			data.push_back({
				.r_offset = rels[i].offset,
				.r_info = ELF32_R_INFO(symbol, rels[i].type),
			});
		}

		relocation_sections.push_back({
			.target = target,
			.data = data,
		});
	}

	Elf32_Section index_of_section(const char* name) const {
		Elf32_Section i = 0;
		for (auto s = sections.begin(); s != sections.end(); s++, i++) {
			if (0 == strcmp(&section_strings.data()[s->sh_name], name)) {
				return i;
			}
		}
		return 0;
	}

	uint32_t id_of_symbol(const char* name) const {
		uint32_t i = 0;
		for (auto s = private_symbols.begin(); s != private_symbols.end(); s++, i++) {
			if (0 == strcmp(&symbol_strings.data()[s->st_name], name)) {
				return i;
			}
		}
		for (auto s = public_symbols.begin(); s != public_symbols.end(); s++, i++) {
			if (0 == strcmp(&symbol_strings.data()[s->st_name], name)) {
				return i;
			}
		}
		return 0;
	}
};

struct Object {
	ObjectImpl* impl;
};

struct Object* object_start(const char* out_file_name) {
	struct Object* retval = new struct Object();
	retval->impl = new ObjectImpl(out_file_name);
	return retval;
}

void object_finish(struct Object* self) {
	delete self->impl;
	delete self;
}

void object_push_file_copy_section(
	struct Object* self,
	const char* src_filename,
	const struct variable_template variable
) {
	self->impl->push_file_copy_section(src_filename, variable);
}

void object_push_bytes_section(
	struct Object* self,
	const void* data, Elf32_Word size,
	const struct variable_template variable
) {
	self->impl->push_bytes_section(data, size, variable);
}

void push_relocation_section(
	struct Object* self,
	const char* target_name,
	const struct relocation_template* rels, size_t rel_count
) {
	self->impl->push_relocation_section(target_name, rels, rel_count);
}
