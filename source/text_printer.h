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

typedef struct {
	uint8_t background : 4;
	uint8_t light : 4;
	uint8_t shadow : 4;
	uint8_t dark : 4;
	bool write_background : 1;
} font_colors_t;

void bg_print(
	tile_4bpp_t* buffer,
	const struct shadow_tiles_window_allocate* window_args,
	const struct font* font,
	coord16_t start_point,
	coord16_t kerning,
	font_colors_t colors,
	const char* message);


#endif        //  #ifndef TEXT_PRINTER_H
