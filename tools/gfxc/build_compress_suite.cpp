#include "build_compress_suite.hpp"

#include <algorithm>
#include <filesystem>
#include "resource_type/background.hpp"
#include "resource_type/tileset.hpp"
#include "choose_compression.hpp"
#include "compare_mismatch_error_message.hpp"
#include "image.hpp"
#include "object.hpp"
#include "png_deserialize.hpp"
#include "resource_type.hpp"
#include "variable_name_for_image.hpp"

using namespace std::string_literals;

struct decompression_suite {
	std::string data_name;
	uint32_t size;
};

void suite_1(std::string_view variable_name, std::string_view label, std::vector<uint8_t> raw, Object& elf) {
	std::vector<struct decompression_suite> choices;

	std::string label_name(variable_name);
	label_name += ".Label";
	{
		std::vector<uint8_t> label2(label.begin(), label.end());
		label2.push_back(0);
		elf.push_single_variable_rodata_sections({label_name, STB_LOCAL}, label2);
	}

	std::string raw_name(variable_name);
	raw_name += ".RAW";
	{
		std::vector<uint8_t> ident;
		ident.clear();
		ident.push_back(0);
		ident.push_back(raw.size());
		ident.push_back(raw.size() >> 8);
		ident.push_back(raw.size() >> 16);
		std::copy(raw.begin(), raw.end(), std::back_inserter(ident));

		std::string ident_name(variable_name);
		ident_name += ".IDENT";
		elf.push_single_variable_rodata_sections({ident_name, STB_LOCAL}, ident);
		choices.emplace_back(ident_name, ident.size());

		elf.push_symbol({
			.name = raw_name,
			.st_value = 4,
			.st_size = static_cast<uint32_t>(raw.size()),
			.binding = STB_LOCAL,
			.type = STT_OBJECT,
			.section = ".rodata."s + ident_name,
		});

	}

	for (choosable_compression compression_alg : compression_algs) {
		auto compressedOpt = compression_alg.compress(raw);
		if (! compressedOpt)
			continue;

		std::vector<uint8_t> compressed = *compressedOpt;

		std::vector<uint8_t> round = compression_alg.decompress(compressed, false);
		if (raw != round) {
			throw std::logic_error(compareMismatchErrorMessage(raw, compressed, round, compression_alg.alg_name, variable_name));
		}

		std::string choice_name(variable_name);
		choice_name += ".";
		choice_name += compression_alg.alg_name;
		elf.push_single_variable_rodata_sections({choice_name, STB_LOCAL}, compressed);
		choices.emplace_back(choice_name, compressed.size());
	}

	std::vector<uint32_t> bytes;
	std::vector<relocation_template> relocs;
	for (size_t i = 0; i < choices.size(); i++) {
		bytes.push_back(0);
		bytes.push_back(0);
		bytes.push_back(0);
		bytes.push_back(choices[i].size);
		relocs.push_back((relocation_template) {
			.offset = static_cast<Elf32_Addr>(16 * i),
			.type = R_ARM_ABS32,
			.symbol_name = label_name,
		});
		relocs.push_back((relocation_template) {
			.offset = static_cast<Elf32_Addr>(16 * i + 4),
			.type = R_ARM_ABS32,
			.symbol_name = raw_name,
		});
		relocs.push_back((relocation_template) {
			.offset = static_cast<Elf32_Addr>(16 * i + 8),
			.type = R_ARM_ABS32,
			.symbol_name = choices[i].data_name,
		});
	}

	{
		std::string data_section_name(".decompression_suite_array.");
		data_section_name += variable_name;

		elf.push_bytes_section(
			{
				.sh_name = data_section_name,
				.sh_type = SHT_PROGBITS,
				.sh_flags = SHF_ALLOC,
				.sh_addralign = 4,
			},
			bytes
		);

		elf.push_relocation_section(data_section_name, relocs);
	}
}

int build_trivial_decompression_suite(std::filesystem::path objfile) {
	Object elf(objfile);

	std::vector<uint8_t> data;
	suite_1("empty", "empty", data, elf);

	for (unsigned i = 1; i <= 4; i++) {
		data.push_back(0);
		std::string var("zeros_");
		std::string label(" zeros");
		char count0[] = {'0', static_cast<char>('0' + i), '\0'};
		std::string count(count0);

		suite_1(var + count, count + label, data, elf);
	}

	{
		for (unsigned i = 4; i < 32; i++) {
			data.push_back(0);
		}
		std::string var("zeros_");
		std::string label(" zeros");
		std::string count("32");

		suite_1(var + count, count + label, data, elf);
	}

	{
		for (unsigned i = 32; i < 2048; i++) {
			data.push_back(0);
		}
		std::string var("zeros_");
		std::string label(" zeros");
		std::string count("2048");

		suite_1(var + count, count + label, data, elf);
	}

	{
		unsigned i;
		data.clear();
		for (i = 0; i < 7; i++) {
			data.push_back(i);
		}
		std::string var("increment8_");
		std::string label(" increment8");
		std::string count("7");

		suite_1(var + count, count + label, data, elf);

		for (; i < 256; i++) {
			data.push_back(i);
		}
		count = "256";

		suite_1(var + count, count + label, data, elf);

		for (; i < 260; i++) {
			data.push_back(i);
		}
		count = "260";

		suite_1(var + count, count + label, data, elf);
	}

	{
		unsigned i;
		data.clear();
		for (i = 0; i < 8; i++) {
			data.push_back(260 - i);
		}
		std::string var("decrement8_wrap");
		std::string label(" decrement8_wrap");
		std::string count("even");

		suite_1(var + count, count + label, data, elf);

		data.clear();
		for (i = 0; i < 8; i++) {
			data.push_back(261 - i);
		}
		count = "odd";

		suite_1(var + count, count + label, data, elf);
	}

	{
		data.clear();
		for (unsigned i = 0; i < 256; i++) {
			data.push_back(i);
			data.push_back(20);
		}
		std::string var("increment16_");
		std::string label(" increment16");
		std::string count("256");

		suite_1(var + count, count + label, data, elf);
	}

	return 0;
}

int build_decompression_suite(std::filesystem::path srcfile, std::filesystem::path objfile) {
	bufferedimage parsed = png_deserialize(srcfile);
	std::pair<std::filesystem::path, bufferedimage> name_and_parsed = std::make_pair(srcfile.filename(), parsed);
	std::string variable_name = variable_name_for_image(name_and_parsed);

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

			suite_1("decompression_suite_"s + variable_name, variable_name, imgdata, elf);
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

			suite_1("decompression_suite_"s + variable_name, variable_name, tiledata, elf);
		}
		break;
	case TYPE_BACKGROUND:
		{
			palette_data_builder pal_builder = background_type_functions.extract_palettes(name_and_parsed);
			pal_builder.condense_colors();
			palette_data pal_data(0, pal_builder);

			std::vector<gbatile_4bpp> tiles = background_type_functions.extract_tiles(name_and_parsed, pal_data);
			std::vector<uint8_t> tiles_bytes;
			for (gbatile_4bpp tile : tiles) {
				for (uint8_t b : tile.bytes()) {
					tiles_bytes.push_back(b);
				}
			}
			tiles_data tiles_data(0, tiles);

			std::string tileset_name = "decompression_suite_"s + variable_name + "_tiles";
			std::string tileset_label = variable_name + " (tiles)";
			suite_1(tileset_name, tileset_label, tiles_bytes, elf);

			std::vector<bg_tile_t> tilemap = background_extract_map(name_and_parsed, pal_data, tiles_data);
			std::vector<uint8_t> tilemap_bytes;
			for (bg_tile_t entry : tilemap) {
				for (uint8_t b : entry.to_bytes()) {
					tilemap_bytes.push_back(b);
				}
			}

			std::string tilemap_variable_name = "decompression_suite_"s + variable_name + "_map";
			std::string tilemap_label = variable_name + " (map)";
			suite_1(tilemap_variable_name, tilemap_label, tilemap_bytes, elf);
		}
		break;
	case TYPE_TILESET:
		{
			palette_data_builder pal_builder = tileset_type_functions.extract_palettes(name_and_parsed);
			pal_builder.condense_colors();
			palette_data pal(0, pal_builder);

			std::vector<gbatile_4bpp> tiles = tileset_type_functions.extract_tiles(name_and_parsed, pal);
			std::vector<uint8_t> tiles_bytes;
			for (gbatile_4bpp tile : tiles) {
				for (uint8_t b : tile.bytes()) {
					tiles_bytes.push_back(b);
				}
			}

			std::string tileset_name = "decompression_suite_"s + variable_name;
			std::string tileset_label = variable_name;
			suite_1(tileset_name, tileset_label, tiles_bytes, elf);
		}
		break;
	default:
		break;
	}

	return 0;
}
