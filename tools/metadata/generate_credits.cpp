#include "generate_credits.h"

#include "extract_metadata.h"
#include "object.hpp"

struct offsets {
	uint32_t title;
	uint32_t author;
	uint32_t author_url;
	uint32_t retrieved_from;
	uint32_t licensed_under;
};

const Elf32_Shdr_Template strings_section_header {
	.sh_name = ".rodata.resource_credits.strs",
	.sh_type = SHT_PROGBITS,
	.sh_flags = SHF_ALLOC,
	.sh_addralign = 1,
};

const variable_template main_symbol("resource_credits", STB_GLOBAL);
const std::string end_symbol_name("resource_credits_end");

void generate_credits_object(
	const char* out_file_name,
	size_t metadata_count,
	const struct metadata* metadatas
) {

	StringTableBuilder strings;
	std::vector<offsets> credits(metadata_count);
	std::vector<relocation_template> credits_relocs;

	for (size_t i = 0; i < metadata_count; i++) {
		credits[i].title = strings.find_or_push(metadatas[i].title ? metadatas[i].title : "\0");
		credits[i].author = strings.find_or_push(metadatas[i].author ? metadatas[i].author : "\0");
		credits[i].author_url = strings.find_or_push(metadatas[i].author_url ? metadatas[i].author_url : "\0");
		credits[i].retrieved_from = strings.find_or_push(metadatas[i].retrieved_from ? metadatas[i].retrieved_from : "\0");
		credits[i].licensed_under = strings.find_or_push(metadatas[i].licensed_under ? metadatas[i].licensed_under : "\0");

		for (size_t j = 0; j < 5; j++) {
			credits_relocs.emplace_back((i * 5 + j) * 4, R_ARM_ABS32, strings_section_header.sh_name);
		}
	}

	Object out_file(out_file_name);

	out_file.push_bytes_section(strings_section_header, strings.to_bytes());

	out_file.push_symbol((Elf32_Sym_Template) {
		.name = strings_section_header.sh_name,
		.st_value = 0x00,
		.st_size = 0x00,
		.binding = STB_LOCAL,
		.type = STT_SECTION,
		.section = strings_section_header.sh_name,
	});

	out_file.push_single_variable_rodata_sections(
		main_symbol,
		credits,
		credits_relocs
	);

	out_file.push_symbol((Elf32_Sym_Template) {
		.name = end_symbol_name,
		.st_value = size_in_bytes(credits),
		.st_size = 0x00,
		.binding = STB_GLOBAL,
		.type = STT_OBJECT,
		.section = ".rodata.resource_credits",
	});
}
