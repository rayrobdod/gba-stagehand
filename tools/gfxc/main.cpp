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
#include "choose_compression.hpp"
#include "find_palette_superset.hpp"
#include "image.hpp"
#include "indexed_insert_only_set.hpp"
#include "object.h"
#include "png_deserialize.hpp"
#include "resource_type.hpp"
#include "subword_output_iterator.hpp"
#include "variable_name_for_image.hpp"

static const unsigned FIRST_TAG = 0x1000;

int write_types_header(std::filesystem::path headerfile) {
	std::ofstream headerstream(headerfile);
	headerstream << "#include \"gba/palette.h\"" << std::endl;
	headerstream << "#include \"gba/oam.h\"" << std::endl;

	font::write_struct(headerstream);
	sprite::write_struct(headerstream);
	background::write_struct(headerstream);

	headerstream << std::endl
		<< "struct bitpacked_tileset {" << std::endl
		<< "	uint16_t size;" << std::endl
		<< "	uint16_t unit_width;" << std::endl
		<< "	char data[];" << std::endl
		<< "};" << std::endl;

	return 0;
}

int compile_object(std::filesystem::path srcdir, std::filesystem::path objfile, std::filesystem::path headerfile) {
	std::map<std::filesystem::path, struct bufferedimage> sprite_imgs;
	std::map<std::filesystem::path, struct bufferedimage> tileset_imgs;
	std::map<std::filesystem::path, struct bufferedimage> monochrome_tileset_imgs;
	std::map<std::filesystem::path, struct bufferedimage> background_imgs;
	std::map<std::filesystem::path, struct bufferedimage> background_mode3_imgs;
	std::map<std::filesystem::path, struct bufferedimage> font_imgs;

	for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{srcdir}) {
		if (dir_entry.is_regular_file() && dir_entry.path().extension() == ".png") {
			std::filesystem::path relative_png = dir_entry.path().lexically_relative(srcdir);

			try {
				bufferedimage result = png_deserialize(dir_entry.path().c_str());
				std::pair nameImage(relative_png, result);

				switch (resource_type(result)) {
				case TYPE_SPRITE:
					sprite_imgs.insert(nameImage);
					break;
				case TYPE_FONT:
					font_imgs.insert(nameImage);
					break;
				case TYPE_TILESET:
					tileset_imgs.insert(nameImage);
					break;
				case TYPE_TILESET_MONOCHROME:
					monochrome_tileset_imgs.insert(nameImage);
					break;
				case TYPE_BACKGROUND:
					background_imgs.insert(nameImage);
					break;
				case TYPE_BACKGROUND_MODE3:
					background_mode3_imgs.insert(nameImage);
					break;
				}
			} catch (const std::exception& e) {
				std::cerr << relative_png << ": " << e.what() << std::endl;
			}
		}
	}

	indexed_insert_only_set<std::vector<rgba16_t>> single_palettes;
	{
		std::set<std::set<rgba16_t>> single_palettes_0;
		for (auto const& image : sprite_imgs) {
			std::set<rgba16_t> new_pal;
			new_pal.insert(image.second.background().with_alpha(0));
			new_pal.merge(image.second.palette());
			if (new_pal.size() > 16) {
				std::string msg(image.first.string());
				msg += ": palette larger than 16 colors";
				throw std::logic_error(msg);
			}
			single_palettes_0.insert(new_pal);
		}

		for (auto const& image : tileset_imgs) {
			std::set<rgba16_t> new_pal;
			new_pal.insert(image.second.background().with_alpha(0));
			new_pal.merge(image.second.palette());
			if (new_pal.size() > 16) {
				std::string msg(image.first.string());
				msg += ": palette larger than 16 colors";
				throw std::logic_error(msg);
			}
			single_palettes_0.insert(new_pal);
		}

		// TODO: more palette deduplication
		// E.g. finding subsets

		for (auto pal0 : single_palettes_0) {
			std::vector<rgba16_t> pal(pal0.begin(), pal0.end());
			single_palettes.find_or_push_back(pal);
		}
	}

	indexed_insert_only_set<std::vector<uint8_t>> tiledatas;
	std::vector<sprite> sprites;

	for (auto const& image : sprite_imgs) {
		uint16_t paltag = find_palette_superset<indexed_insert_only_set<std::vector<rgba16_t>>>(single_palettes, image.second.palette());
		const std::vector<rgba16_t> used_pal = single_palettes[paltag];
		paltag += FIRST_TAG;

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
		uint16_t tiletag = FIRST_TAG + tiledatas.find_or_push_back(tiledata);

		std::string name = variable_name_for_image(image);

		sprites.push_back({name, paltag, tiletag, size});

		std::map<std::string, std::vector<rgba16_t>> altpals = image.second.alt_palettes(used_pal);
		for (auto altpal : altpals) {
			std::string altname(name);
			altname += "_";
			altname += altpal.first;
			uint16_t altpaltag = FIRST_TAG + single_palettes.find_or_push_back(altpal.second);
			sprites.push_back({altname, altpaltag, tiletag, size});
		}
	}

	std::vector<background> backgrounds;
	for (auto const& image : background_imgs) {
		backgrounds.emplace_back(image);
	}

	std::vector<font> fonts;
	for (auto const& image : font_imgs) {
		fonts.emplace_back(image);
	}

	struct Object* elf = object_start(objfile.c_str());
	std::ofstream headerstream(headerfile);

	for (auto pal0 = single_palettes.cbegin(); pal0 != single_palettes.cend(); ++pal0) {
		size_t i = pal0 - single_palettes.cbegin();
		char pal_name[16];
		snprintf(pal_name, 16, "plte.%lx", FIRST_TAG + i);
		std::vector<rgb15_t> pal;
		for (auto color : *pal0) {
			pal.push_back(color.strip_alpha());
		}
		object_push_bytes_section(elf, pal.data(), sizeof(rgba16_t) * pal.size(), {pal_name, STB_LOCAL});
	}

	for (size_t tiletag = 0; tiletag < tiledatas.size(); ++tiletag) {
		char tiles_name[16];
		snprintf(tiles_name, 16, "tile.%lx", FIRST_TAG + tiletag);
		auto compressed = choose_compression(tiles_name, tiledatas[tiletag]);

		if (false) {
			std::cout << "  " << tiles_name << ": " << compressed.alg_name << std::endl;
		}

		object_push_bytes_section(elf, compressed.data.data(), sizeof(uint8_t) * compressed.data.size(), {tiles_name, STB_LOCAL});
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
	for (auto const& image : tileset_imgs) {
		std::string name = variable_name_for_image(image);

		const std::set<rgba16_t> innate_pal = image.second.palette();
		uint16_t paltag;
		for (paltag = 0; paltag < single_palettes.size(); ++paltag) {
			const std::vector<rgba16_t> check_pal = single_palettes[paltag];

			if (std::includes(check_pal.begin(), check_pal.end(), innate_pal.begin(), innate_pal.end())) {
				break;
			}
		}
		if (paltag >= single_palettes.size()) {
			std::string msg("Lost this sprite's palette");
			throw std::logic_error(msg);
		}
		const std::vector<rgba16_t> used_pal = single_palettes[paltag];
		paltag += FIRST_TAG;
		char pal_name[16];
		snprintf(pal_name, 16, "plte.%x", paltag);

		uint16_t tile_count = 0;
		subword_output_iterator<uint8_t, uint4_t, DIRECTION_INC> tiledata_builder;
		for (auto subimg : image.second.subs(8, 8)) {
			for (auto pixel : subimg.pixels()) {
				auto palptr = std::find(used_pal.begin(), used_pal.end(), pixel);
				uint4_t palindex(palptr - used_pal.begin());

				*tiledata_builder = palindex;
				++tiledata_builder;
			}
			++tile_count;
		}
		std::vector tiledata = tiledata_builder.result();

		std::string tiles_name("tile.");
		tiles_name += name;

		object_push_bytes_section(elf, tiledata.data(), sizeof(uint8_t) * tiledata.size(), {tiles_name.c_str(), STB_GLOBAL});

		std::array<uint32_t, 4> serialized = {
			1,
			0,
			tile_count,
			0,
		};

		std::array<relocation_template, 2> relocs;
		relocs[0] = (struct relocation_template){
			.offset = 4,
			.type = R_ARM_ABS32,
			.symbol_name = pal_name,
		};
		relocs[1] = (struct relocation_template){
			.offset = 12,
			.type = R_ARM_ABS32,
			.symbol_name = tiles_name.c_str(),
		};

		object_push_bytes_section(elf, serialized.data(), sizeof(uint32_t) * serialized.size(), {name.c_str(), STB_GLOBAL});
		push_relocation_section(elf, name.c_str(), relocs.data(), relocs.size());

		headerstream << "extern const struct tileset_graphics " << name << ";" << std::endl;
	}

	headerstream << std::endl;
	for (auto const& image : monochrome_tileset_imgs) {
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

		object_push_bytes_section(elf, tiledata.data(), sizeof(uint8_t) * tiledata.size(), {name.c_str(), STB_GLOBAL});

		headerstream << "extern const struct bitpacked_tileset " << name << ";" << std::endl;
	}

	headerstream << std::endl;
	for (auto const& image : background_mode3_imgs) {
		std::string name = variable_name_for_image(image);

		if (image.second.width() != 240 || image.second.height() != 160) {
			std::string msg(image.first.string());
			msg += ": mode3 background does not have expected dimensions";
			throw std::logic_error(msg);
		}

		std::vector<rgba16_t> imgdata(image.second.pixels().begin(), image.second.pixels().end());

		object_push_bytes_section(elf, imgdata.data(), sizeof(rgba16_t) * imgdata.size(), {name.c_str(), STB_GLOBAL});

		headerstream << "extern const rgb_t " << name << "[160][240];" << std::endl;
	}

	headerstream.close();
	object_finish(elf);

	return 0;
}

int main(int argc, char* argv[]) {
	using namespace std::string_view_literals;

	if (argc == 3 && "--structs"sv == argv[1]) {
		return write_types_header(argv[2]);
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
