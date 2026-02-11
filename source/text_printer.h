#ifndef TEXT_PRINTER_H
#define TEXT_PRINTER_H

#include <stdbool.h>
#include <stdint.h>
#include "management/shadow_vram.h"

struct font;

typedef struct {
	int16_t x;
	int16_t y;
} coord16_t;

__attribute__((aligned(4)))
typedef struct {
	uint8_t background : 4;
	uint8_t light : 4;
	uint8_t shadow : 4;
	uint8_t dark : 4;
	bool write_background : 1;
	uint16_t : 15;
} font_colors_t;

enum text_print_step_retval {
	/// Can continue to next step
	TEXT_PRINT_STEP_CONTINUE,
	/// End of string
	TEXT_PRINT_STEP_STOP,
	/// Request wait for user input
	TEXT_PRINT_STEP_WAIT,
};

enum text_print_overflow_x {
	TEXTPRINTOVERFLOWX_CLIP = 0,
	TEXTPRINTOVERFLOWX_WRAPAROUND,
};

enum text_print_overflow_y {
	TEXTPRINTOVERFLOWY_CLIP = 0,
	TEXTPRINTOVERFLOWY_WRAPAROUND,
	TEXTPRINTOVERFLOWY_SCROLL,
};

typedef struct {
	enum text_print_overflow_x x;
	enum text_print_overflow_y y;
} text_print_overflow_t;

[[maybe_unused]]
static const text_print_overflow_t TEXTPRINTOVERFLOW_CLIP = {
		TEXTPRINTOVERFLOWX_CLIP, TEXTPRINTOVERFLOWY_CLIP};
[[maybe_unused]]
static const text_print_overflow_t TEXTPRINTOVERFLOW_WRAPAROUND = {
		TEXTPRINTOVERFLOWX_WRAPAROUND, TEXTPRINTOVERFLOWY_WRAPAROUND};

struct text_print_step_state {
	coord16_t current_point;
	uint8_t scroll_up;
	uint8_t height_since_last_wait;
	const char* message;

	volatile tile_4bpp_t* buffer;
	const struct shadow_tiles_window_allocate* window_args;
	const struct font* font;
	coord16_t start_point;
	coord16_t kerning;
	text_print_overflow_t overflow;
	font_colors_t colors;
};

__attribute__((access(read_write, 1)))
enum text_print_step_retval text_print_step(
	struct text_print_step_state*);

__attribute__((access(write_only, 1)))
void text_print_step_init(
	struct text_print_step_state*,
	volatile tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	coord16_t start_point,
	coord16_t kerning,
	text_print_overflow_t overflow,
	font_colors_t colors,
	const char* message);

/// Print the `message` onto the `buffer` using the other args
void text_print_immediate(
	volatile tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	coord16_t start_point,
	coord16_t kerning,
	font_colors_t colors,
	const char* message);

void text_clear_immediate(
	volatile tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	// inclusive
	coord16_t start_point,
	// exclusive
	coord16_t end_point,
	uint8_t color);

/// The width in pixels of `message` when using the other args
__attribute__((pure, nonnull))
unsigned text_width(
	const struct font* font,
	coord16_t kerning,
	const char* message);

#endif        //  #ifndef TEXT_PRINTER_H
