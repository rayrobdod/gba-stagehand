#include "text_printer.h"

#include "graphics.h"
#include "mgba.h"

static void set_one_pixel(
	tile_4bpp_t* tiles,
	unsigned tiles_width,
	unsigned tiles_height,
	int pixel_x,
	int pixel_y,
	unsigned new_value
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

void bg_print(
	tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	coord16_t start_point,
	coord16_t kerning,
	font_colors_t colors,
	const char* message) {

	const int height = font->glyph_height;
	int x = start_point.x;
	int y = start_point.y;
	char c = *message;

	for (; '\0' != (c = *message); message++) {
		if (c >= 32 && (c - 32) < font->glyph_count) {
			const int width = font->glyphs[c - 32].width;
			const uint8_t* pixel_data = font->pixel_data + font->glyphs[c - 32].pixel_data_start_index;
			unsigned subword_index = 0;
			unsigned pixel_data_word;

			for (int dy = 0; dy < height; dy++)
			for (int dx = 0; dx < width; dx++) {
				if (subword_index == 0) {
					subword_index = 3;
					pixel_data_word = *pixel_data;
					pixel_data++;
				} else {
					subword_index--;
					pixel_data_word = pixel_data_word >> 2;
				}

				if (0 != (pixel_data_word & 0x3) || colors.write_background) {
					uint8_t new_color = 0;
					switch(pixel_data_word & 0x3) {
					case 0:
						new_color = colors.background;
						break;
					case 1:
						new_color = colors.light;
						break;
					case 2:
						new_color = colors.shadow;
						break;
					case 3:
						new_color = colors.dark;
						break;
					}

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

			x += width + kerning.x;
		}
	}

}

