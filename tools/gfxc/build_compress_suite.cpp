#include "build_compress_suite.hpp"

#include <filesystem>
#include "resource_type/background.hpp"
#include "choose_compression.hpp"
#include "image.hpp"
#include "object.hpp"
#include "png_deserialize.hpp"
#include "resource_type.hpp"
#include "variable_name_for_image.hpp"

struct compression_suite {
	std::string data_name;
	uint32_t size;
};

void suite_1(std::string_view name, std::vector<uint8_t> raw, Object& elf) {
	std::vector<struct compression_suite> choices;

	{
		std::vector<uint8_t> ident;
		ident.clear();
		ident.push_back(0);
		ident.push_back(raw.size());
		ident.push_back(raw.size() >> 8);
		ident.push_back(raw.size() >> 16);
		std::copy(raw.begin(), raw.end(), std::back_inserter(ident));

		std::string ident_name(name);
		ident_name += ".IDENT";
		elf.push_single_variable_rodata_sections({ident_name, STB_LOCAL}, ident);
		choices.emplace_back(ident_name, ident.size());
	}

	for (choosable_compression compression_alg : compression_algs) {
		auto compressedOpt = compression_alg.compress(raw);
		if (! compressedOpt)
			continue;

		std::vector<uint8_t> compressed = *compressedOpt;
		std::string choice_name(name);
		choice_name += ".";
		choice_name += compression_alg.alg_name;
		elf.push_single_variable_rodata_sections({choice_name, STB_LOCAL}, compressed);
		choices.emplace_back(choice_name, compressed.size());
	}

	std::vector<uint32_t> bytes;
	std::vector<relocation_template> relocs;
	for (size_t i = 0; i < choices.size(); i++) {
		bytes.push_back(0);
		bytes.push_back(choices[i].size);
		relocs.push_back((relocation_template) {
			.offset = static_cast<Elf32_Addr>(8 * i),
			.type = R_ARM_ABS32,
			.symbol_name = choices[i].data_name.c_str(),
		});
	}
	bytes.push_back(0);
	bytes.push_back(0);

	elf.push_single_variable_rodata_sections({name, STB_GLOBAL}, bytes, relocs);
}

int build_compression_suite(std::filesystem::path srcfile, std::filesystem::path objfile) {
	bufferedimage parsed = png_deserialize(srcfile);
	std::pair<std::filesystem::path, bufferedimage> name_and_parsed = std::make_pair(srcfile.filename(), parsed);
	std::string name("compression_suite_");
	name += variable_name_for_image(name_and_parsed);

	Object elf(objfile);

	switch (resource_type(parsed)) {
	case TYPE_BACKGROUND_MODE3:
		{
			std::vector<uint8_t> imgdata;
			for (auto pixel : parsed.pixels()) {
				for (uint8_t byte : pixel.to_bytes()) {
					imgdata.push_back(byte);
				}
			}

			suite_1(name, imgdata, elf);
		}
		break;
	case TYPE_SPRITE:
		{
			// This won't be exactly the same as retail, but it should have the same frequencies
			std::set<rgba16_t> pal0 = parsed.palette();
			std::vector<rgba16_t> pal(pal0.begin(), pal0.end());

			std::vector<uint8_t> tiledata;
			for (auto subimg : parsed.subs(8, 8)) {
				for (auto b : subimg.to_tile_4bpp(pal).bytes()) {
					tiledata.push_back(b);
				}
			}

			suite_1(name, tiledata, elf);
		}
		break;
	case TYPE_BACKGROUND:
		{
			background bg(name_and_parsed);

			std::string tileset_name = name + "_tiles";
			suite_1(tileset_name, bg.tileset, elf);

			std::string tilemap_name = name + "_map";
			std::vector<uint8_t> tilemap_bytes;
			for (auto entry : bg.tilemap) {
				for (uint8_t byte : entry.to_bytes()) {
					tilemap_bytes.push_back(byte);
				}
			}
			suite_1(tilemap_name, tilemap_bytes, elf);
		}
		break;
	default:
		break;
	}

	return 0;
}
