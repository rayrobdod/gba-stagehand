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
#include "build_compress_suite.hpp"
#include "choose_compression.hpp"
#include "image.hpp"
#include "indexed_insert_only_set.hpp"
#include "object.hpp"
#include "png_deserialize.hpp"
#include "resource_type.hpp"
#include "resource_type_functions.hpp"
#include "subword_output_iterator.hpp"
#include "variable_name_for_image.hpp"

static const unsigned FIRST_TAG = 0x1000;

static std::string palette_name_for_image(std::pair<std::filesystem::path, struct bufferedimage> image) {
	std::string retval = "plte.";
	auto palette_name_pair = image.second.text().find("Palette Tag");
	if (palette_name_pair != image.second.text().end()) {
		retval += palette_name_pair->second;
	} else {
		retval += variable_name_for_image(image);
		retval += "$";
	}
	return retval;
}

static std::string tileset_name_for_image(std::pair<std::filesystem::path, struct bufferedimage> image) {
	std::string retval = "tiles.";
	auto palette_name_pair = image.second.text().find("Tileset Tag");
	if (palette_name_pair != image.second.text().end()) {
		retval += palette_name_pair->second;
	} else {
		retval += variable_name_for_image(image);
		retval += "$";
	}
	return retval;
}

int write_types_header(std::filesystem::path headerfile) {
	std::ofstream headerstream(headerfile);
	headerstream << "#ifndef GRAPHICS_TYPES" << std::endl;
	headerstream << "#define GRAPHICS_TYPES" << std::endl << std::endl;

	headerstream << "#include \"gba/palette.h\"" << std::endl;
	headerstream << "#include \"gba/oam.h\"" << std::endl;
	headerstream << "struct CompressedData;" << std::endl;

	for (auto pair : type_functionss) {
		type_functions fns = pair.second;
		if (fns.write_struct) {
			fns.write_struct(headerstream);
		}
	};

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
	std::map<const std::string, const palette_data> palette_datas;
	{
		std::map<const std::string, palette_data_builder> palette_data_builders;
		for (auto images : sorted_imgs) {
			const type typ = images.first;
			auto fns_ptr = type_functionss.find(typ);
			if (fns_ptr != type_functionss.end()) {
				type_functions fns = fns_ptr->second;

				if (fns.extract_palettes) {
					for (auto image : images.second) {
						std::string palette_name = palette_name_for_image(image);
						file_to_palette_name.emplace(image.first, palette_name);

						auto out = palette_data_builders.try_emplace(palette_name).first;
						palette_data_builder new_data = fns.extract_palettes(image);

						out->second.merge(new_data, palette_name);
					}
				}
			}
		}

		for (auto builders_i = palette_data_builders.begin(); builders_i != palette_data_builders.end(); builders_i++) {
			builders_i->second.condense_colors();
		}

		uint16_t next_paltag = FIRST_TAG;
		for (auto builder_pair : palette_data_builders) {
			std::string palname = builder_pair.first;
			palette_data_builder builder = builder_pair.second;

			std::map<const std::string, alt_palette_data> alternates;
			for (auto alternate : builder.alternates) {
				alternates.try_emplace(alternate.first, next_paltag++, alternate.second);
			}

			std::vector<std::vector<rgba16_t>> colorss;
			for (auto colors : builder.colorss) {
				colorss.emplace_back(colors.begin(), colors.end());
			}

			palette_datas.try_emplace(palname, next_paltag++, colorss, alternates);
		}
	}

	std::map<const std::filesystem::path, const std::string> file_to_tiles_name;
	std::map<const std::string, const tiles_data> tiles_datas;
	{
		std::map<const std::string, std::vector<gbatile_4bpp>> tileset_data_builders;
		for (auto images : sorted_imgs) {
			const type typ = images.first;
			auto fns_ptr = type_functionss.find(typ);
			if (fns_ptr != type_functionss.end()) {
				type_functions fns = fns_ptr->second;

				if (fns.extract_tiles) {
					for (auto image : images.second) {
						const std::string tileset_name = tileset_name_for_image(image);
						file_to_tiles_name.emplace(image.first, tileset_name);

						const std::string palette_name = file_to_palette_name.at(image.first);
						palette_data palette = palette_datas.at(palette_name);

						auto out = tileset_data_builders.try_emplace(tileset_name).first;
						std::vector<gbatile_4bpp> new_data = fns.extract_tiles(image, palette);

						for (gbatile_4bpp tile : new_data) {
							out->second.push_back(tile);
						}
					}
				}
			}
		}

		uint16_t next_tiletag = FIRST_TAG;
		for (auto builder_pair : tileset_data_builders) {
			std::string tilesname = builder_pair.first;
			std::vector<gbatile_4bpp> builder = builder_pair.second;

			tiles_datas.try_emplace(tilesname, next_tiletag++, builder);
		}
	}


	Object elf(objfile);
	std::ofstream headerstream(headerfile);

	for (auto const& palette_data : palette_datas) {
		std::string var_name = palette_data.first;

		std::vector<rgba16_t> pala;
		std::vector<rgb15_t> pal;
		for (auto colors : palette_data.second.colorss) {
			for (auto color : colors) {
				pala.push_back(color);
				pal.push_back(color.strip_alpha());
			}
			for (size_t i = colors.size(); i < 16; i++) {
				pala.push_back(rgba16_t::BLACK);
				pal.push_back(rgb15_t::BLACK);
			}
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

	for (auto const& tileset_data : tiles_datas) {
		std::string var_name = tileset_data.first;

		std::vector<uint8_t> bytes;
		for (gbatile_4bpp tile : tileset_data.second.tiles) {
			for (uint8_t b : tile.bytes()) {
				bytes.push_back(b);
			}
		}

		auto compressed = choose_compression(tileset_data.first, bytes);

		elf.push_single_variable_rodata_sections({var_name, STB_LOCAL}, compressed.data);
	}

	{
		for (auto images : sorted_imgs) {
			const type typ = images.first;
			auto fns_ptr = type_functionss.find(typ);
			if (fns_ptr != type_functionss.end()) {
				type_functions fns = fns_ptr->second;

				if (fns.write_to_elf) {
					for (auto image : images.second) {
						std::string var_name = variable_name_for_image(image);

						std::pair<std::string, palette_data> my_palette_data;
						{
							auto palette_name_ptr = file_to_palette_name.find(image.first);
							if (palette_name_ptr != file_to_palette_name.end()) {
								std::string palette_name = palette_name_ptr->second;
								auto palette_ptr = palette_datas.find(palette_name);
								if (palette_ptr != palette_datas.end()) {
									my_palette_data = *palette_ptr;
								}
							}
						}

						std::pair<std::string, tiles_data> my_tiles_data;
						{
							auto tiles_name_ptr = file_to_tiles_name.find(image.first);
							if (tiles_name_ptr != file_to_tiles_name.end()) {
								std::string tiles_name = tiles_name_ptr->second;
								auto tiles_ptr = tiles_datas.find(tiles_name);
								if (tiles_ptr != tiles_datas.end()) {
									my_tiles_data = *tiles_ptr;
								}
							}
						}

						fns.write_to_elf(
							image,
							my_palette_data,
							my_tiles_data,
							var_name,
							headerstream,
							elf
						);
					}
				}
			}
		}
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
