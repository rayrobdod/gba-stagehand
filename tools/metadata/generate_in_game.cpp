#include "generate_in_game.h"

#include "extract_metadata.h"
#include "object.hpp"

struct tasl_offsets {
	uint32_t title;
	uint32_t author;
	uint32_t author_url;
	uint32_t retrieved_from;
	uint32_t licensed_under;
};
struct metadata_offsets {
	struct tasl_offsets primary_tasl;
	struct tasl_offsets derived_from_tasl;
};

static const Elf32_Shdr_Template strings_section_header {
	.sh_name = ".rodata.resource_credits.strs",
	.sh_type = SHT_PROGBITS,
	.sh_flags = SHF_ALLOC,
	.sh_addralign = 1,
};

static const variable_template main_symbol("resource_credits", STB_GLOBAL);
static const std::string end_symbol_name("resource_credits_end");

static const char* elvis(const char* a) {
	return (a ? a : "\0");
}

void generate_credits_object(
	const char* out_file_name,
	size_t metadata_count,
	const struct metadata_and_filenames* metadatas
) {
	StringTableBuilder strings;
	std::vector<metadata_offsets> credits(metadata_count);
	std::vector<relocation_template> credits_relocs;

	for (size_t i = 0; i < metadata_count; i++) {
		credits[i].primary_tasl.title = strings.find_or_push(elvis(metadatas[i].metadata.tasl.title));
		credits[i].primary_tasl.author_url = strings.find_or_push(elvis(metadatas[i].metadata.tasl.author_url));
		credits[i].primary_tasl.author = strings.find_or_push(elvis(metadatas[i].metadata.tasl.author));
		credits[i].primary_tasl.retrieved_from = strings.find_or_push(elvis(metadatas[i].metadata.tasl.retrieved_from));
		credits[i].primary_tasl.licensed_under = strings.find_or_push(elvis(metadatas[i].metadata.tasl.licensed_under));
		credits[i].derived_from_tasl.title = strings.find_or_push(elvis(metadatas[i].metadata.derived_from_tasl.title));
		credits[i].derived_from_tasl.author_url = strings.find_or_push(elvis(metadatas[i].metadata.derived_from_tasl.author_url));
		credits[i].derived_from_tasl.author = strings.find_or_push(elvis(metadatas[i].metadata.derived_from_tasl.author));
		credits[i].derived_from_tasl.retrieved_from = strings.find_or_push(elvis(metadatas[i].metadata.derived_from_tasl.retrieved_from));
		credits[i].derived_from_tasl.licensed_under = strings.find_or_push(elvis(metadatas[i].metadata.derived_from_tasl.licensed_under));

		for (size_t j = 0; j < 10; j++) {
			credits_relocs.emplace_back((i * 10 + j) * 4, R_ARM_ABS32, strings_section_header.sh_name);
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
