#include "png_deserialize.hpp"

#include <cstdbool>
#include <cstdlib>
#include <cstring>
#include <png.h>
#include <stdexcept>

static const unsigned magic_size = 8;

static const unsigned MASK_5_BIT = 0x1F;
static const unsigned MASK_1_BIT = 0x1;
static const unsigned MASK_2_BIT = 0x3;
static const unsigned MASK_4_BIT = 0xF;
static const unsigned MASK_8_BIT = 0xFF;
static const unsigned MASK_16_BIT = 0xFFFF;

static constexpr uint8_t scale_1bit_to_5bit(unsigned input) {
	return (input ? MASK_5_BIT : 0);
}
static uint8_t scale_2bit_to_5bit(unsigned input) {
	return (input * MASK_5_BIT + MASK_2_BIT / 2) / MASK_2_BIT;
}
static uint8_t scale_4bit_to_5bit(unsigned input) {
	return (input * MASK_5_BIT + MASK_4_BIT / 2) / MASK_4_BIT;
}
static uint8_t scale_8bit_to_5bit(unsigned input) {
	return (input * MASK_5_BIT + MASK_8_BIT / 2) / MASK_8_BIT;
}
static uint8_t scale_16bit_to_5bit(unsigned input) {
	return (input * MASK_5_BIT + MASK_16_BIT / 2) / MASK_16_BIT;
}

static unsigned read_sample_1bit(png_byte* row, unsigned i) {
	return (row[i / 8] >> (7 - (i % 8))) & MASK_1_BIT;
}
static unsigned read_sample_2bit(png_byte* row, unsigned i) {
	return (row[i / 4] >> ((3 - (i % 4)) * 2)) & MASK_2_BIT;
}
static unsigned read_sample_4bit(png_byte* row, unsigned i) {
	return (row[i / 2] >> ((1 - (i % 2)) * 4)) & MASK_4_BIT;
}
static unsigned read_sample_8bit(png_byte* row, unsigned i) {
	return row[i];
}
static unsigned read_sample_16bit(png_byte* row, unsigned i) {
	return row[i * 2] << 8 | row[i * 2 + 1];
}

struct bitfuncs {
	unsigned mask;
	uint8_t (*scale_to_5bit)(unsigned input);
	unsigned (*read_sample)(png_byte* row, unsigned i);
};

static const struct bitfuncs bit1 = {
	.mask = MASK_1_BIT,
	.scale_to_5bit = scale_1bit_to_5bit,
	.read_sample = read_sample_1bit,
};
static const struct bitfuncs bit2 = {
	.mask = MASK_2_BIT,
	.scale_to_5bit = scale_2bit_to_5bit,
	.read_sample = read_sample_2bit,
};
static const struct bitfuncs bit4 = {
	.mask = MASK_4_BIT,
	.scale_to_5bit = scale_4bit_to_5bit,
	.read_sample = read_sample_4bit,
};
static const struct bitfuncs bit8 = {
	.mask = MASK_8_BIT,
	.scale_to_5bit = scale_8bit_to_5bit,
	.read_sample = read_sample_8bit,
};
static const struct bitfuncs bit16 = {
	.mask = MASK_16_BIT,
	.scale_to_5bit = scale_16bit_to_5bit,
	.read_sample = read_sample_16bit,
};

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
	png_uint_32 transforms = 0;

	png_read_png(png_ptr, info_ptr, transforms, NULL);

	png_uint_32 width;
	png_uint_32 height;
	int bit_depth;
	int color_type;

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	png_text* texts;
	unsigned num_text = png_get_text(png_ptr, info_ptr, &texts, NULL);

	std::map<std::string, std::string> text;

	for (unsigned i = 0; i < num_text; i++) {
		text.insert(std::pair(texts[i].key, texts[i].text));
	}

	std::vector<rgba16_t> pixels(width * height);

	png_byte** row_pointers = png_get_rows(png_ptr, info_ptr);

	const struct bitfuncs* bit_depth_funcs;
	switch (bit_depth) {
	case 1:
		bit_depth_funcs = &bit1;
		break;
	case 2:
		bit_depth_funcs = &bit2;
		break;
	case 4:
		bit_depth_funcs = &bit4;
		break;
	case 8:
		bit_depth_funcs = &bit8;
		break;
	case 16:
		bit_depth_funcs = &bit16;
		break;
	default:
		{
			png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
			fclose(fp);
			std::string msg("invalid bit_depth: ");
			msg += std::to_string(bit_depth);
			throw std::runtime_error(msg);
		}
	}

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

			for (unsigned y = 0; y < height; y++) {
				for (unsigned x = 0; x < width; x++) {
					int palindex = bit_depth_funcs->read_sample(row_pointers[y], x);

					if (palindex < palette_length) {
						pixels[x + y * width] = (rgba16_t){
							.r = scale_8bit_to_5bit(palette[palindex].red),
							.g = scale_8bit_to_5bit(palette[palindex].green),
							.b = scale_8bit_to_5bit(palette[palindex].blue),
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
		}
		break;

	case PNG_COLOR_TYPE_GRAY:
		{
			png_color_16* transparency;
			bool has_transparency = png_get_tRNS(png_ptr, info_ptr, NULL, NULL, &transparency);
			if (has_transparency) {
				transparency->gray &= bit_depth_funcs->mask;
			}

			for (unsigned y = 0; y < height; y++) {
				for (unsigned x = 0; x < width; x++) {
					unsigned g1 = bit_depth_funcs->read_sample(row_pointers[y], x);
					uint8_t a = ! (has_transparency && g1 == transparency->gray);
					uint8_t g = bit_depth_funcs->scale_to_5bit(g1);
					pixels[x + y * width] = (rgba16_t){g, g, g, a};
				}
			}
		}
		break;

	case PNG_COLOR_TYPE_GRAY_ALPHA:
		{
			for (unsigned y = 0; y < height; y++) {
				for (unsigned x = 0; x < width; x++) {
					unsigned g1 = bit_depth_funcs->read_sample(row_pointers[y], x * 2);
					unsigned a1 = bit_depth_funcs->read_sample(row_pointers[y], x * 2 + 1);

					uint8_t a = bit_depth_funcs->mask == a1;
					uint8_t g = bit_depth_funcs->scale_to_5bit(g1);
					pixels[x + y * width] = (rgba16_t){g, g, g, a};
				}
			}
		}
		break;

	case PNG_COLOR_TYPE_RGB:
		{
			png_color_16* transparency;
			bool has_transparency = png_get_tRNS(png_ptr, info_ptr, NULL, NULL, &transparency);
			if (has_transparency) {
				transparency->red &= bit_depth_funcs->mask;
				transparency->green &= bit_depth_funcs->mask;
				transparency->blue &= bit_depth_funcs->mask;
			}

			for (unsigned y = 0; y < height; y++) {
				for (unsigned x = 0; x < width; x++) {
					unsigned r8 = bit_depth_funcs->read_sample(row_pointers[y], x * 3);
					unsigned g8 = bit_depth_funcs->read_sample(row_pointers[y], x * 3 + 1);
					unsigned b8 = bit_depth_funcs->read_sample(row_pointers[y], x * 3 + 2);
					uint8_t a = ! (has_transparency && r8 == transparency->red && g8 == transparency->green && b8 == transparency->blue);
					uint8_t r = bit_depth_funcs->scale_to_5bit(r8);
					uint8_t g = bit_depth_funcs->scale_to_5bit(g8);
					uint8_t b = bit_depth_funcs->scale_to_5bit(b8);
					pixels[x + y * width] = (rgba16_t){r, g, b, a};
				}
			}
		}
		break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
		{
			for (unsigned y = 0; y < height; y++) {
				for (unsigned x = 0; x < width; x++) {
					unsigned r8 = bit_depth_funcs->read_sample(row_pointers[y], x * 4);
					unsigned g8 = bit_depth_funcs->read_sample(row_pointers[y], x * 4 + 1);
					unsigned b8 = bit_depth_funcs->read_sample(row_pointers[y], x * 4 + 2);
					unsigned a8 = bit_depth_funcs->read_sample(row_pointers[y], x * 4 + 3);

					uint8_t a = bit_depth_funcs->mask == a8;
					uint8_t r = bit_depth_funcs->scale_to_5bit(r8);
					uint8_t g = bit_depth_funcs->scale_to_5bit(g8);
					uint8_t b = bit_depth_funcs->scale_to_5bit(b8);
					pixels[x + y * width] = (rgba16_t){r, g, b, a};
				}
			}
		}
		break;

	default:
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		std::string msg("Unknown paltype ");
		msg += std::to_string(color_type);
		throw std::runtime_error(msg);
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return bufferedimage(width, height, pixels, text);
}