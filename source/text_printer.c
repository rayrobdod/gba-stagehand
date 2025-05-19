#include "text_printer.h"

#include "graphics.h"
#include "mgba.h"

#define max(a, b) (a > b ? a : b)

_Static_assert(sizeof(uint32_t) == sizeof(font_colors_t));
union font_colors_2_uint {
	font_colors_t colors;
	uint32_t uint;
};

static void set_one_pixel(
	tile_4bpp_t* tiles,
	unsigned tiles_width,
	unsigned tiles_height,
	int pixel_x,
	int pixel_y,
	uint32_t new_value
) {
	new_value &= 0xF;

	int tile_x = pixel_x / 8;
	int tile_y = pixel_y / 8;
	if (tile_x < 0) return;
	if (tile_y < 0) return;
	if (tile_x >= tiles_width) return;
	if (tile_y >= tiles_height) return;
	unsigned tileid = tile_y * tiles_width + tile_x;

	pixel_x %= 8;
	pixel_y %= 8;
	unsigned shortid = (pixel_y * 2 + pixel_x / 4);
	if (pixel_x % 4 == 0) {
		tiles[tileid][shortid] &= 0xFFF0;
		tiles[tileid][shortid] |= new_value;
	} else if (pixel_x % 4 == 1) {
		tiles[tileid][shortid] &= 0xFF0F;
		tiles[tileid][shortid] |= new_value << 4;
	} else if (pixel_x % 4 == 2) {
		tiles[tileid][shortid] &= 0xF0FF;
		tiles[tileid][shortid] |= new_value << 8;
	} else {
		tiles[tileid][shortid] &= 0x0FFF;
		tiles[tileid][shortid] |= new_value << 12;
	}
}

void text_print(
	tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	coord16_t start_point,
	coord16_t kerning,
	font_colors_t colors,
	const char* message) {

	const int glyph_height = font->glyph_height;
	int x = start_point.x;
	int y = start_point.y;

	const union font_colors_2_uint colors_u = {.colors = colors};

	for (char c; '\0' != (c = *message); message++) {
		if (c == '\n') {
			x = start_point.x;
			y += glyph_height + kerning.y;
		} else
		if (c >= 32 && (c - 32) < font->glyph_count) {
			const int glyph_width = font->glyphs[c - 32].width;
			const uint8_t* input_data = font->pixel_data + font->glyphs[c - 32].pixel_data_start_index;
			unsigned subword_index = 0;
			unsigned input_data_word;

			for (int dy = 0; dy < glyph_height; dy++)
			for (int dx = 0; dx < glyph_width; dx++) {
				if (subword_index == 0) {
					subword_index = 3;
					input_data_word = *input_data;
					input_data++;
				} else {
					subword_index--;
					input_data_word = input_data_word >> 2;
				}

				if (0 != (input_data_word & 0x3) || colors_u.colors.write_background) {
					uint8_t new_color = (colors_u.uint)
						>> (4 * (input_data_word & 0x3)) & 0xF;

					set_one_pixel(
						buffer,
						window_args->width,
						window_args->height,
						x + dx,
						y + dy,
						new_color
					);
				}
			}

			x += glyph_width + kerning.x;
		}
	}
}

unsigned text_width(
	const struct font* font,
	coord16_t kerning,
	const char* message) {

	unsigned x = 0;
	unsigned max_x = 0;

	for (char c; '\0' != (c = *message); message++) {
		if (c == '\n') {
			x = 0;
		} else
		if (c >= 32 && (c - 32) < font->glyph_count) {
			const int width = font->glyphs[c - 32].width;

			x += width + kerning.x;
			max_x = max(max_x, x);
		}
	}

	return max_x;
}
