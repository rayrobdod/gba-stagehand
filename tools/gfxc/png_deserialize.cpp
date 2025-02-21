#include "png_deserialize.hpp"

#include <cstdbool>
#include <cstdlib>
#include <cstring>
#include <png.h>
#include <stdexcept>

static const unsigned magic_size = 8;

bufferedimage png_deserialize(
	const std::string& name) {
	FILE* fp = fopen(name.c_str(), "rb");
	if (! fp) {
		throw std::runtime_error("Could not open file");
	}

	png_byte magic[magic_size];
	if (magic_size != fread(magic, 1, magic_size, fp)) {
		throw std::runtime_error("Could not read");
	};

	bool is_png = ! png_sig_cmp(magic, 0, magic_size);
	if (! is_png) {
		throw std::runtime_error("Not PNG");
	}

	png_structp png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		(png_voidp)NULL,
		NULL,
		NULL);
	if (! png_ptr) {
		throw std::runtime_error("Could not allocate png_struct");
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (! info_ptr) {
		png_destroy_read_struct(
			&png_ptr,
			(png_infopp)NULL,
			(png_infopp)NULL);
		throw std::runtime_error("Could not allocate png_info");
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		throw std::runtime_error("Could not set setjmp");
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, magic_size);
	png_set_user_limits(png_ptr, 0x1000, 0x1000);
	png_uint_32 transforms =
		PNG_TRANSFORM_PACKING |
		PNG_TRANSFORM_SCALE_16;

	png_read_png(png_ptr, info_ptr, transforms, NULL);

	png_uint_32 width;
	png_uint_32 height;
	int color_type;

	png_get_IHDR(png_ptr, info_ptr, &width, &height, NULL, &color_type, NULL, NULL, NULL);

	png_text* texts;
	unsigned num_text = png_get_text(png_ptr, info_ptr, &texts, NULL);

	std::map<std::string, std::string> text;

	for (unsigned i = 0; i < num_text; i++) {
		text.insert(std::pair(texts[i].key, texts[i].text));
	}

	std::vector<rgba16_t> pixels(width * height);

	png_byte** row_pointers = png_get_rows(png_ptr, info_ptr);

	switch (color_type) {
	case PNG_COLOR_TYPE_PALETTE:
		{
			png_byte* transparency;
			int transparency_length;
			png_color* palette;
			int palette_length;
			png_get_PLTE(png_ptr, info_ptr, &palette, &palette_length);
			if (! png_get_tRNS(png_ptr, info_ptr, &transparency, &transparency_length, NULL)) {
				transparency_length = 0;
			}

			for (unsigned y = 0; y < height; y++)
				for (unsigned x = 0; x < width; x++) {
					int palindex = row_pointers[y][x];

					if (palindex < palette_length) {
						pixels[x + y * width] = (rgba16_t){
							.r = static_cast<uint8_t>(palette[palindex].red / 8),
							.g = static_cast<uint8_t>(palette[palindex].green / 8),
							.b = static_cast<uint8_t>(palette[palindex].blue / 8),
							.a = 1,
						};
					} else {
						png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
						fclose(fp);
						throw std::runtime_error("Invalid palette entry");
					}
					if (palindex < transparency_length && transparency[palindex] != 255) {
						pixels[x + y * width].a = 0;
					}
				}
		}
		break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
		{
			for (unsigned y = 0; y < height; y += 4)
				for (unsigned x = 0; x < width; x++) {
					png_byte r = row_pointers[y][x];
					png_byte g = row_pointers[y][x + 1];
					png_byte b = row_pointers[y][x + 2];
					png_byte a = row_pointers[y][x + 3];

					pixels[x + y * width] = (rgba16_t){
						.r = static_cast<uint8_t>(r / 8),
						.g = static_cast<uint8_t>(g / 8),
						.b = static_cast<uint8_t>(b / 8),
						.a = a != 255,
					};
				}
		}
		break;

	default:
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		std::string msg("TODO paltype ");
		msg += std::to_string(color_type);
		throw std::runtime_error(msg);
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return bufferedimage(width, height, pixels, text);
}