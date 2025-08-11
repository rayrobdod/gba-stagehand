#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include "resource_type/background.hpp"
#include "resource_type/font.hpp"
#include "resource_type/sprite.hpp"
#include "resource_type/tileset.hpp"
#include "build_compress_suite.hpp"
#include "choose_compression.hpp"
#include "find_palette_superset.hpp"
#include "image.hpp"
#include "indexed_insert_only_set.hpp"
#include "object.hpp"
#include "png_deserialize.hpp"
#include "resource_type.hpp"
#include "subword_output_iterator.hpp"
#include "variable_name_for_image.hpp"

static const unsigned FIRST_TAG = 0x1000;

struct alt_palette_data {
	const uint16_t tag;
	std::map<rgba16_t, rgba16_t> mapping;

	alt_palette_data(uint16_t _tag) : tag(_tag) {}
};

struct palette_data {
	const uint16_t tag;
	std::set<rgba16_t> colors;
	std::map<const std::string, alt_palette_data> alternates;

	palette_data(uint16_t _tag) : tag(_tag) {}
};

int write_types_header(std::filesystem::path headerfile) {
	std::ofstream headerstream(headerfile);
	headerstream << "#ifndef GRAPHICS_TYPES" << std::endl;
	headerstream << "#define GRAPHICS_TYPES" << std::endl << std::endl;

	headerstream << "#include \"gba/palette.h\"" << std::endl;
	headerstream << "#include \"gba/oam.h\"" << std::endl;
	headerstream << "struct CompressedData;" << std::endl;

	background::write_struct(headerstream);
	font::write_struct(headerstream);
	sprite::write_struct(headerstream);
	tileset::write_struct(headerstream);

	headerstream << std::endl
		<< "struct bitpacked_tileset {" << std::endl
		<< "	uint16_t size;" << std::endl
		<< "	uint16_t unit_width;" << std::endl
		<< "	char data[];" << std::endl
		<< "};" << std::endl
		 << std::endl;

	headerstream << "#endif" << std::endl;

	return 0;
}

int compile_object(std::filesystem::path srcdir, std::filesystem::path objfile, std::filesystem::path headerfile) {
	std::map<type, std::map<std::filesystem::path, struct bufferedimage>> sorted_imgs;

	for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{srcdir}) {
		if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".png") {
			std::filesystem::path relative_png = dir_entry.path().lexically_relative(srcdir);

			try {
				bufferedimage result = png_deserialize(dir_entry.path().c_str());
				std::pair nameImage(relative_png, result);
				type type = resource_type(result);

				sorted_imgs.try_emplace(type);
				sorted_imgs.at(type).insert(nameImage);
			} catch (const std::exception& e) {
				std::cerr << relative_png << ": " << e.what() << std::endl;
			}
		}
	}

	std::map<const std::filesystem::path, const std::string> file_to_palette_name;
	std::map<const std::string, palette_data> palette_datas;
	{
		uint16_t next_paltag = FIRST_TAG;
		for (auto const& image : sorted_imgs.at(TYPE_SPRITE)) {
			std::string palette_name;
			auto palette_name_pair = image.second.text().find("Palette Tag");
			if (palette_name_pair != image.second.text().end()) {
				palette_name = palette_name_pair->second;
			} else {
				palette_name = variable_name_for_image(image);
				palette_name += "$";
			}

			file_to_palette_name.emplace(image.first, palette_name);

			auto palette_data_emplace_result = palette_datas.try_emplace(palette_name, next_paltag);
			if (palette_data_emplace_result.second) {
				++next_paltag;
			}
			auto palette_data_ptr = palette_data_emplace_result.first;

			palette_data_ptr->second.colors.insert(image.second.background().with_alpha(0));
			palette_data_ptr->second.colors.merge(image.second.palette());

			if (palette_data_ptr->second.colors.size() > 16) {
				std::string msg(palette_name);
				msg += ": palette larger than 16 colors";
				throw std::logic_error(msg);
			}

			std::map<std::string, std::map<rgba16_t, rgba16_t>> addend_alts = image.second.alt_palettes();
			for (auto const& addend_alt : addend_alts) {
				std::string alt_name = addend_alt.first;
				std::map<rgba16_t, rgba16_t> addend_replacements = addend_alt.second;

				auto altpalette_data_emplace_result = palette_data_ptr->second.alternates.try_emplace(alt_name, next_paltag);
				if (altpalette_data_emplace_result.second) {
					++next_paltag;
				}
				auto altpalette_data_ptr = altpalette_data_emplace_result.first;

				for (auto const& addend_replacement : addend_replacements) {
					rgba16_t from = addend_replacement.first;
					rgba16_t to = addend_replacement.second;

					auto i = altpalette_data_ptr->second.mapping.find(from);
					if (i == altpalette_data_ptr->second.mapping.end()) {
						altpalette_data_ptr->second.mapping.emplace(from, to);
					} else if (to == i->second) {
						// do nothing
					} else {
						std::string msg;
						msg += palette_name;
						msg += "_";
						msg += alt_name;
						msg += ": contradictory alternate palette mapping";
						throw std::logic_error(msg);
					}
				}
			}
		}
	}

	std::vector<sprite> sprites;

	{
		uint16_t next_tiletag = FIRST_TAG;
		for (auto const& image : sorted_imgs.at(TYPE_SPRITE)) {
			std::string var_name = variable_name_for_image(image);

			std::string pal_name = file_to_palette_name.find(image.first)->second;
			palette_data pal_data = palette_datas.find(pal_name)->second;

			std::vector<std::pair<uint16_t, std::string>> palettes;
			palettes.emplace_back(pal_data.tag, "");
			for (auto alt : pal_data.alternates) {
				std::string altname = "_";
				altname += alt.first;
				palettes.emplace_back(alt.second.tag, altname);
			}

			const std::vector<rgba16_t> used_pal(pal_data.colors.begin(), pal_data.colors.end());

			subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> tiledata_builder;
			for (auto subimg : image.second.subs(8, 8)) {
				for (auto pixel : subimg.pixels()) {
					auto palptr = std::find(used_pal.begin(), used_pal.end(), pixel);
					uint4_t palindex(palptr - used_pal.begin());

					*tiledata_builder = palindex;
					++tiledata_builder;
				}
			}
			std::vector<uint8_t> tiledata = tiledata_builder.result();

			enum sprite_size size = sprite_size(image.second.width(), image.second.height());

			sprites.push_back({var_name, pal_name, palettes, next_tiletag++, tiledata, size});
		}
	}

	std::vector<background> backgrounds;
	for (auto const& image : sorted_imgs.at(TYPE_BACKGROUND)) {
		backgrounds.emplace_back(image);
	}

	std::vector<font> fonts;
	for (auto const& image : sorted_imgs.at(TYPE_FONT)) {
		fonts.emplace_back(image);
	}

	std::vector<tileset> tilesets;
	for (auto const& image : sorted_imgs.at(TYPE_TILESET)) {
		tilesets.emplace_back(image);
	}

	Object elf(objfile);
	std::ofstream headerstream(headerfile);

	for (auto const& palette_data : palette_datas) {
		std::string var_name = "plte.";
		var_name += palette_data.first;

		std::vector<rgba16_t> pala;
		std::vector<rgb15_t> pal;
		for (auto color : palette_data.second.colors) {
			pala.push_back(color);
			pal.push_back(color.strip_alpha());
		}
		while (false) { //(pal.size() < 16) {
			pala.push_back(rgba16_t::BLACK);
			pal.push_back(rgb15_t::BLACK);
		}
		elf.push_single_variable_rodata_sections({var_name, STB_LOCAL}, pal);

		for (auto alt : palette_data.second.alternates) {
			std::string alt_var_name = var_name;
			alt_var_name += "_";
			alt_var_name += alt.first;

			std::vector<rgb15_t> alt_pal;
			for (rgba16_t main_color : pala) {
				auto alt_color = alt.second.mapping.find(main_color);
				if (alt.second.mapping.end() == alt_color) {
					alt_pal.push_back(main_color.strip_alpha());
				} else {
					alt_pal.push_back(alt_color->second.strip_alpha());
				}
			}
			elf.push_single_variable_rodata_sections({alt_var_name, STB_LOCAL}, alt_pal);
		}
	}

	headerstream << std::endl;
	for (font font : fonts) {
		font.write(headerstream, elf);
	}

	headerstream << std::endl;
	for (sprite sprite : sprites) {
		sprite.write(headerstream, elf);
	}

	headerstream << std::endl;
	for (auto const& background : backgrounds) {
		background.write(headerstream, elf);
	}

	headerstream << std::endl;
	for (auto const& tileset : tilesets) {
		tileset.write(headerstream, elf);
	}

	headerstream << std::endl;
	for (auto const& image : sorted_imgs.at(TYPE_TILESET_MONOCHROME)) {
		std::string name = variable_name_for_image(image);

		subword_output_iterator<uint8_t, uint1_t, DIRECTION_INC> tiledata_builder;
		for (auto subimg : image.second.subs(8, 8)) {
			for (auto pixel : subimg.pixels()) {
				uint1_t palindex(pixel == (rgba16_t){0, 0, 0, 1} ? 1 : 0);

				*tiledata_builder = palindex;
				++tiledata_builder;
			}
		}
		std::vector tiledata = tiledata_builder.result();
		unsigned size = tiledata.size();
		tiledata.insert(tiledata.begin(), 0);
		tiledata.insert(tiledata.begin(), 1);
		tiledata.insert(tiledata.begin(), (sizeof(uint8_t) * size) >> 8);
		tiledata.insert(tiledata.begin(), sizeof(uint8_t) * size);

		elf.push_single_variable_rodata_sections({name, STB_GLOBAL}, tiledata);

		headerstream << "extern const struct bitpacked_tileset " << name << ";" << std::endl;
	}

	headerstream << std::endl;
	for (auto const& image : sorted_imgs.at(TYPE_BACKGROUND_MODE3)) {
		std::string name = variable_name_for_image(image);

		if (image.second.width() != 240 || image.second.height() != 160) {
			std::string msg(image.first.string());
			msg += ": mode3 background does not have expected dimensions";
			throw std::logic_error(msg);
		}

		std::vector<rgba16_t> imgdata(image.second.pixels().begin(), image.second.pixels().end());

		elf.push_single_variable_rodata_sections({name, STB_GLOBAL}, imgdata);

		headerstream << "extern const rgb_t " << name << "[160][240];" << std::endl;
	}

	headerstream.close();

	return 0;
}

int main(int argc, char* argv[]) {
	using namespace std::string_view_literals;

	if (argc == 3 && "--structs"sv == argv[1]) {
		return write_types_header(argv[2]);
	} else
	if (argc == 4 && "--decompression_suite"sv == argv[1]) {
		if ("--trivial"sv == argv[2]) {
			return build_trivial_decompression_suite(argv[3]);
		} else {
			return build_decompression_suite(argv[2], argv[3]);
		}
	} else
	if (argc == 4) {
		std::filesystem::path srcdir = argv[1];
		std::filesystem::path objfile = argv[2];
		std::filesystem::path headerfile = argv[3];

		return compile_object(srcdir, objfile, headerfile);
	} else
	{
		printf("TODO \n");
		return 0;
	}
}
