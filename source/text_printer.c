#include "text_printer.h"

#include "gba/screen.h"
#include "utils/minmax.h"
#include "graphics_types.h"
#include "mgba.h"

static unsigned text_width_until_next_break_opportunity(
	const struct font* font,
	coord16_t kerning,
	const char* message);

_Static_assert(sizeof(uint32_t) == sizeof(font_colors_t));
union font_colors_2_uint {
	font_colors_t colors;
	uint32_t uint;
};

static inline int divfloor(int numerator, int denominator) {
	return ((numerator / denominator) - (numerator < 0 && numerator % denominator ? 1 : 0));
}
static inline int divceil(int numerator, int denominator) {
	return ((numerator / denominator) + (numerator > 0 && numerator % denominator ? 1 : 0));
}
static inline int modfloor(int numerator, int denominator) {
	return ((numerator % denominator) + denominator) % denominator;
}

__attribute__((always_inline))
static inline int text_print_one_glyph(
	volatile tile_4bpp_t* buffer,
	int x,
	int y,
	const struct font_glyph* font_glyph,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	text_print_overflow_t overflow,
	font_colors_t colors
) {
	const int glyph_height = font->glyph_height;
	const int glyph_width = font_glyph->width;
	const unsigned tiles_width = window_args->width;

	const uint16_t* input_data = font->pixel_data + font_glyph->pixel_data_start_index;

	for (int dy = 0; dy < glyph_height; dy++) {
		int pixel_y = y + dy;
		int tile_y = divfloor(pixel_y, 8);
		int subtile_y = modfloor(pixel_y, 8);

		if (0 > tile_y || tile_y >= window_args->height) {
			if (overflow.y == TEXTPRINTOVERFLOWY_WRAPAROUND) {
				tile_y %= window_args->height;
				tile_y += window_args->height;
				tile_y %= window_args->height;
			} else {
				input_data += divceil(glyph_width, 4);
				continue;
			}
		}

		for (int dx = 0; dx < glyph_width; dx += 4) {
			int pixel_x = x + dx;
			int tile_x = divfloor(pixel_x, 8);
			int subtile_x = modfloor(pixel_x, 8);

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

			if (0 > tile_x || tile_x >= window_args->width) {
				if (overflow.x == TEXTPRINTOVERFLOWX_WRAPAROUND) {
					tile_x %= window_args->width;
					tile_x += window_args->width;
					tile_x %= window_args->width;
				}
			}

			//MgbaPrintf(MGBA_LOG_INFO, "x: %d, %d, %d ; y: %d, %d, %d", pixel_x, tile_x, subtile_x, pixel_y, tile_y, subtile_y);

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

			if (0 > tile_x || tile_x >= window_args->width) {
				if (overflow.x == TEXTPRINTOVERFLOWX_WRAPAROUND) {
					tile_x %= window_args->width;
					tileid -= window_args->width;
				}
			}

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

typedef struct {
	bool success;
	uint8_t bytes_read;
	uint16_t glyph_index;
} next_glyph_retval_t;

static const next_glyph_retval_t next_glyph_retval_failure = {false, 0, 0};

static inline next_glyph_retval_t next_glyph_0(const char* message, const struct font* font, unsigned cmap_index) {
	uint8_t c = (uint8_t) *message;
	const struct font_byte_to_glyph_trie* cmap_entry = &(font->byte_to_glyph_trie[cmap_index]);

	if (c < cmap_entry->start) {
		return next_glyph_retval_failure;
	}
	else if (c <= cmap_entry->end) {
		if (cmap_entry->byte_to_glyph_index) {
			next_glyph_retval_t retval = next_glyph_0(message + 1, font, cmap_entry->byte_to_glyph_index + (c - cmap_entry->start));
			if (retval.success) {
				retval.bytes_read += 1;
				return retval;
			}
		}

		if (0xFFFF == cmap_entry->glyphs_start_index) {
			return next_glyph_retval_failure;
		}
		else {
			next_glyph_retval_t retval = {
				true,
				1,
				cmap_entry->glyphs_start_index + (c - cmap_entry->start)
			};
			return retval;
		}
	}
	else {
		return next_glyph_0(message, font, cmap_index + 1);
	}
}

static inline next_glyph_retval_t next_glyph(const char* message, const struct font* font) {
	return next_glyph_0(message, font, 0);
}

static inline enum text_print_step_retval text_print_step_newline(
	struct text_print_step_state* state
) {
	enum text_print_step_retval retval;

	if (state->overflow.y == TEXTPRINTOVERFLOWY_SCROLL) {
		const unsigned window_bottom = TILE_PIXEL_SIDE * state->window_args->height;

		state->current_point.x = state->start_point.x;
		unsigned next_y = state->current_point.y + state->font->glyph_height + state->kerning.y;

		if (next_y + state->font->glyph_height < window_bottom) {
			state->current_point.y = next_y;
		} else {
			state->scroll_up = state->font->glyph_height + state->kerning.y;
		}

		state->height_since_last_wait += state->font->glyph_height + state->kerning.y;
		if (state->height_since_last_wait < window_bottom) {
			retval = TEXT_PRINT_STEP_CONTINUE;
		} else {
			state->height_since_last_wait = state->font->glyph_height;
			retval = TEXT_PRINT_STEP_WAIT;
		}
	} else {
		state->current_point.x = state->start_point.x;
		state->current_point.y += state->font->glyph_height + state->kerning.y;
		retval = TEXT_PRINT_STEP_CONTINUE;
	}

	return retval;
}

enum text_print_step_retval text_print_step(
	struct text_print_step_state* state
) {
	if (state->scroll_up) {
		state->scroll_up--;

		int pixel_y;
		for (pixel_y = state->start_point.y; pixel_y < TILE_PIXEL_SIDE * state->window_args->height - 1; pixel_y++) {
			unsigned tile_y = pixel_y / TILE_PIXEL_SIDE;
			unsigned subtile_y = pixel_y % TILE_PIXEL_SIDE;
			unsigned tile_offset = (subtile_y == 7 ? state->window_args->width : 0);
			int offset = (subtile_y == 7 ? -14 : 2);

			for (unsigned pixel_x = 0; pixel_x < TILE_PIXEL_SIDE * state->window_args->width; pixel_x += TILE_PIXEL_SIDE / 2) {
				unsigned tile_x = pixel_x / TILE_PIXEL_SIDE;
				unsigned subtile_x = pixel_x % TILE_PIXEL_SIDE;

				unsigned tileid = tile_y * state->window_args->width + tile_x;
				unsigned subtileid = (subtile_y * 2 + subtile_x / 4);

				state->buffer[tileid][subtileid] = state->buffer[tileid + tile_offset][subtileid + offset];
			}
		}

		unsigned tile_y = pixel_y / TILE_PIXEL_SIDE;
		unsigned subtile_y = pixel_y % TILE_PIXEL_SIDE;

		for (unsigned pixel_x = 0; pixel_x < TILE_PIXEL_SIDE * state->window_args->width; pixel_x += TILE_PIXEL_SIDE / 2) {
			unsigned tile_x = pixel_x / TILE_PIXEL_SIDE;
			unsigned subtile_x = pixel_x % TILE_PIXEL_SIDE;

			unsigned tileid = tile_y * state->window_args->width + tile_x;
			unsigned subtileid = (subtile_y * 2 + subtile_x / 4);

			state->buffer[tileid][subtileid] = 0;
		}

		return TEXT_PRINT_STEP_CONTINUE;
	} else {
		char c = *(state->message);
		enum text_print_step_retval retval = TEXT_PRINT_STEP_STOP;

		if (c == '\0') {
			retval = TEXT_PRINT_STEP_STOP;
		} else
		if (c == '\f') {
			if (state->overflow.y == TEXTPRINTOVERFLOWY_SCROLL) {
				state->scroll_up = state->current_point.y + state->font->glyph_height + state->kerning.y;
				state->current_point = state->start_point;
				state->height_since_last_wait = state->font->glyph_height;
			}
			++(state->message);
			retval = TEXT_PRINT_STEP_WAIT;
		} else
		if (c == '\r') {
			state->current_point.x = state->start_point.x;
			++(state->message);
			retval = TEXT_PRINT_STEP_CONTINUE;
		} else
		if (c == '\n') {
			++(state->message);
			retval = text_print_step_newline(state);
		} else
		if (c == ' ' && state->overflow.x == TEXTPRINTOVERFLOWX_WORDWRAP) {
			if (state->current_point.x + text_width_until_next_break_opportunity(state->font, state->kerning, state->message)
					> state->window_args->width * 8) {
				++(state->message);
				retval = text_print_step_newline(state);
			} else {
				next_glyph_retval_t next = next_glyph(state->message, state->font);

				if (next.success) {
					state->current_point.x =
						state->kerning.x +
						text_print_one_glyph(
							state->buffer,
							state->current_point.x, state->current_point.y,
							&state->font->glyphs[next.glyph_index],
							state->window_args,
							state->font,
							state->overflow,
							state->colors);
					state->message += next.bytes_read;
					retval = TEXT_PRINT_STEP_CONTINUE;
				}
			}
		} else
		if ((c == '-' || c == '/' || c == '\\') && state->overflow.x == TEXTPRINTOVERFLOWX_WORDWRAP) {
			next_glyph_retval_t next = next_glyph(state->message, state->font);

			if (next.success) {
				state->current_point.x =
						state->kerning.x +
						text_print_one_glyph(
							state->buffer,
							state->current_point.x, state->current_point.y,
							&state->font->glyphs[c - 32],
							state->window_args,
							state->font,
							state->overflow,
							state->colors);
				state->message += next.bytes_read;
			} else {
				state->message += 1;
			}
			if (state->current_point.x + text_width_until_next_break_opportunity(state->font, state->kerning, state->message)
					> state->window_args->width * 8) {
				retval = text_print_step_newline(state);
			} else {
				retval = TEXT_PRINT_STEP_CONTINUE;
			}
		} else
		{
			next_glyph_retval_t next = next_glyph(state->message, state->font);

			if (next.success) {
				state->current_point.x =
					state->kerning.x +
					text_print_one_glyph(
						state->buffer,
						state->current_point.x, state->current_point.y,
						&state->font->glyphs[next.glyph_index],
						state->window_args,
						state->font,
						state->overflow,
						state->colors);
				state->message += next.bytes_read;
				retval = TEXT_PRINT_STEP_CONTINUE;
			}
		}

		return retval;
	}
}

void text_print_step_init(
	struct text_print_step_state* state,
	volatile tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	coord16_t start_point,
	coord16_t kerning,
	text_print_overflow_t overflow,
	font_colors_t colors,
	const char* message
) {
	state->current_point = start_point;
	state->scroll_up = 0;
	state->height_since_last_wait = font->glyph_height;
	state->message = message;

	state->buffer = buffer;
	state->window_args = window_args;
	state->font = font;
	state->start_point = start_point;
	state->kerning = kerning;
	state->overflow = overflow;
	state->colors = colors;
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
		{
			next_glyph_retval_t next = next_glyph(message, font);

			if (next.success) {
				x = kerning.x + text_print_one_glyph(
					buffer,
					x, y,
					&font->glyphs[next.glyph_index],
					window_args,
					font,
					TEXTPRINTOVERFLOW_CLIP,
					colors);
				message += next.bytes_read - 1;
			}
		}
	}
}

void text_clear_immediate(
	volatile tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	coord16_t start_point,
	coord16_t end_point,
	uint8_t color
) {
	const unsigned tiles_width = window_args->width;
	const unsigned output_paint = 0x1111 * color;

	for (int pixel_y = start_point.y; pixel_y < end_point.y; pixel_y++) {
		int tile_y = divfloor(pixel_y, 8);
		int subtile_y = modfloor(pixel_y, 8);

		for (int pixel_x = start_point.x; pixel_x < end_point.x; pixel_x += 4) {
			int tile_x = divfloor(pixel_x, 8);
			int subtile_x = modfloor(pixel_x, 8);

			unsigned output_mask = 0xFFFF;

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
}

static unsigned text_width_until_next_break_opportunity(
	const struct font* font,
	coord16_t kerning,
	const char* message) {

	unsigned x = 0;
	char c = *message;
	if (c == '\0') { return 0; }

	{
		next_glyph_retval_t next = next_glyph(message, font);
		if (next.success) {
			const int width = font->glyphs[next.glyph_index].width;
			x += width + kerning.x;
			message += next.bytes_read;
		}
		else {
			message += 1;
		}
	}

	while (true) {
		char prev_c = c;
		c = *message;
		if (c == '\0' || c == '\n' || c == ' ') {
			return x;
		} else
		if (prev_c == '/' || prev_c == '\\' || prev_c == '-') {
			return x;
		} else
		{
			next_glyph_retval_t next = next_glyph(message, font);
			if (next.success) {
				const int width = font->glyphs[next.glyph_index].width;
				x += width + kerning.x;
				message += next.bytes_read;
			}
			else {
				message += 1;
			}
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
		{
			next_glyph_retval_t next = next_glyph(message, font);

			if (next.success) {
				const int width = font->glyphs[next.glyph_index].width;
				x += width + kerning.x;
				max_x = max(max_x, x);
				message += next.bytes_read - 1;
			}
		}
	}

	return max_x;
}
