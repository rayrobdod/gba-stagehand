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
#include <vector>
#include "bpp4_output_iterator.hpp"
#include "image.hpp"
#include "indexed_insert_only_set.hpp"
#include "object.h"
#include "png_deserialize.hpp"
#include "resource_type.hpp"
#include "sprite.hpp"

static const unsigned FIRST_TAG = 0x1000;

int main(int argc, char* argv[]) {
	if (argc < 4) {
		printf("TODO \n");
		return 0;
	}

	std::filesystem::path srcdir = argv[1];
	std::filesystem::path objfile = argv[2];
	std::filesystem::path headerfile = argv[3];

	std::map<std::filesystem::path, struct bufferedimage> sprite_imgs;
	std::map<std::filesystem::path, struct bufferedimage> tileset_imgs;
	std::map<std::filesystem::path, struct bufferedimage> scene_imgs;

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
				case TYPE_TILESET:
					tileset_imgs.insert(nameImage);
					break;
				case TYPE_SCENE:
					scene_imgs.insert(nameImage);
					break;
				}
			} catch (const std::exception& e) {
				std::cerr << relative_png << ": " << e.what() << std::endl;
			}
		}
	}

	std::vector<std::vector<rgba16_t>> single_palettes;
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

		// TODO: more deduplication

		for (auto pal0 : single_palettes_0) {
			std::vector pal(pal0.begin(), pal0.end());
			single_palettes.push_back(pal);
		}
	}

	indexed_insert_only_set<std::vector<uint32_t>> tiledatas;
	std::vector<sprite> sprites;

	for (auto const& image : sprite_imgs) {
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

		bpp4_output_iterator tiledata_builder;
		for (auto subimg : image.second.subs(8, 8)) {
			for (auto pixel : subimg.pixels()) {
				auto palptr = std::find(used_pal.begin(), used_pal.end(), pixel);
				unsigned palindex = palptr - used_pal.begin();

				*tiledata_builder = palindex;
				++tiledata_builder;
			}
		}
		std::vector tiledata = tiledata_builder.result();

		enum sprite_size size = sprite_size(image.second.width(), image.second.height());
		uint16_t tiletag = FIRST_TAG + tiledatas.find_or_push_back(tiledata);

		std::string name;
		{
			auto manual = image.second.text().find(std::string("Variable"));
			if (manual != image.second.text().end()) {
				name = manual->second;
			} else {
				std::filesystem::path p = image.first.parent_path();
				for (auto segment : p) {
					name += segment;
					name += "_";
				}
				name += image.first.stem().string();
			}
		}

		sprites.push_back({name, paltag, tiletag, size});
	}

	struct Object* elf = object_start(objfile.c_str());
	std::ofstream headerstream(headerfile);

	for (auto pal0 = single_palettes.begin(); pal0 != single_palettes.end(); ++pal0) {
		size_t i = pal0 - single_palettes.begin();
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
		std::vector<uint32_t> tiledata = tiledatas[tiletag];
		object_push_bytes_section(elf, tiledata.data(), sizeof(uint32_t) * tiledata.size(), {tiles_name, STB_LOCAL});
	}

	for (sprite sprite : sprites) {
		std::array<uint32_t, 4> serialized = {
			0,
			0,
			static_cast<uint32_t>(sprite.paltag | (sprite.tiletag << 16)),
			sprite.size,
		};

		char pal_name[16];
		snprintf(pal_name, 16, "plte.%x", sprite.paltag);
		char tiles_name[16];
		snprintf(tiles_name, 16, "tile.%x", sprite.tiletag);

		std::array<relocation_template, 2> relocs;
		relocs[0] = (struct relocation_template){
			.offset = 0,
			.type = R_ARM_ABS32,
			.symbol_name = pal_name,
		};
		relocs[1] = (struct relocation_template){
			.offset = 4,
			.type = R_ARM_ABS32,
			.symbol_name = tiles_name,
		};

		object_push_bytes_section(elf, serialized.data(), sizeof(uint32_t) * serialized.size(), {sprite.var_name.c_str(), STB_GLOBAL});
		push_relocation_section(elf, sprite.var_name.c_str(), relocs.data(), relocs.size());

		headerstream << "extern const struct shadow_oam_template " << sprite.var_name << ";" << std::endl;
	}

	for (auto const& image : tileset_imgs) {
		std::string name;
		{
			auto manual = image.second.text().find(std::string("Variable"));
			if (manual != image.second.text().end()) {
				name = manual->second;
			} else {
				name = image.first.stem().string();
			}
		}

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
		bpp4_output_iterator tiledata_builder;
		for (auto subimg : image.second.subs(8, 8)) {
			for (auto pixel : subimg.pixels()) {
				auto palptr = std::find(used_pal.begin(), used_pal.end(), pixel);
				unsigned palindex = palptr - used_pal.begin();

				*tiledata_builder = palindex;
				++tiledata_builder;
			}
			++tile_count;
		}
		std::vector tiledata = tiledata_builder.result();

		std::string tiles_name("tile.");
		tiles_name += name;

		object_push_bytes_section(elf, tiledata.data(), sizeof(uint32_t) * tiledata.size(), {tiles_name.c_str(), STB_GLOBAL});

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

	headerstream.close();
	object_finish(elf);

	return 0;
}
