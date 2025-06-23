#include "text_printer.h"

#include "utils/minmax.h"
#include "graphics_types.h"
#include "mgba.h"

_Static_assert(sizeof(uint32_t) == sizeof(font_colors_t));
union font_colors_2_uint {
	font_colors_t colors;
	uint32_t uint;
};

__attribute__((always_inline))
static inline int text_print_one_glyph(
	volatile tile_4bpp_t* buffer,
	int x,
	int y,
	const struct font_glyph* font_glyph,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	font_colors_t colors
) {
	const int glyph_height = font->glyph_height;
	const int glyph_width = font_glyph->width;
	const unsigned tiles_width = window_args->width;

	const uint16_t* input_data = font->pixel_data + font_glyph->pixel_data_start_index;

	for (int dy = 0; dy < glyph_height; dy++) {
		int pixel_y = y + dy;
		int tile_y = pixel_y / 8;
		int subtile_y = pixel_y % 8;

		for (int dx = 0; dx < glyph_width; dx += 4) {
			int pixel_x = x + dx;
			int tile_x = pixel_x / 8;
			int subtile_x = pixel_x % 8;

			unsigned input_word = *input_data;
			input_data++;

			unsigned output_mask = ((input_word >> 0) & 0x1111) * 0xF;
			unsigned output_paint = ((input_word >> 0) & 0x1111) * colors.light;
			output_mask |= ((input_word >> 1) & 0x1111) * 0xF;
			output_paint |= ((input_word >> 1) & 0x1111) * colors.shadow;
			output_mask |= ((input_word >> 2) & 0x1111) * 0xF;
			output_paint |= ((input_word >> 2) & 0x1111) * colors.dark;
			if (colors.write_background) {
				output_mask |= ((input_word >> 3) & 0x1111) * 0xF;
				output_paint |= ((input_word >> 3) & 0x1111) * colors.background;
			}

			unsigned tileid = tile_y * tiles_width + tile_x;
			unsigned subtileid = (subtile_y * 2 + subtile_x / 4);
			unsigned shift = (subtile_x % 4) * 4;

			if (0 <= tile_x && tile_x < window_args->width) {
				uint16_t output_word = buffer[tileid][subtileid];

				uint16_t output_mask1 = output_mask << shift;
				output_word &= ~output_mask1;
				uint16_t output_paint1 = output_paint << shift;
				output_word |= output_paint1;
				buffer[tileid][subtileid] = output_word;
			}

			if (subtile_x == 0 || subtile_x == 4) {
				continue;
			} else if (subtile_x < 4) {
				subtile_x += 4;
				subtileid += 1;
			} else {
				tile_x += 1;
				subtile_x -= 4;
				tileid += 1;
				subtileid -= 1;
			}
			shift = 16 - shift;

			if (0 <= tile_x && tile_x < window_args->width) {
				uint16_t output_word = buffer[tileid][subtileid];

				uint16_t output_mask1 = output_mask >> shift;
				output_word &= ~output_mask1;
				uint16_t output_paint1 = output_paint >> shift;
				output_word |= output_paint1;
				buffer[tileid][subtileid] = output_word;
			}
		}
	}

	return x + glyph_width;
}

void text_print_immediate(
	volatile tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	coord16_t start_point,
	coord16_t kerning,
	font_colors_t colors,
	const char* message
) {
	const int glyph_height = font->glyph_height;
	int x = start_point.x;
	int y = start_point.y;

	for (char c; '\0' != (c = *message); message++) {
		if (c == '\n') {
			x = start_point.x;
			y += glyph_height + kerning.y;
		} else
		if (c >= 32 && (c - 32) < font->glyph_count) {
			x = kerning.x + text_print_one_glyph(
				buffer,
				x, y,
				&font->glyphs[c - 32],
				window_args,
				font,
				colors);
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
