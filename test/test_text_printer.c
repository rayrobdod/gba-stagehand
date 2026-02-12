#include "text_printer.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "harness.h"
#include "main.h"
#include "mgba.h"
#include "graphics.h"
#include "graphics_types.h"

static unsigned get_pixel_at(const tile_4bpp_t* buffer, unsigned tiles_width, unsigned pixel_x, unsigned pixel_y) {
	int tile_y = pixel_y / 8;
	int subtile_y = pixel_y % 8;

	int tile_x = pixel_x / 8;
	int subtile_x = pixel_x % 8;

	unsigned tileid = tile_y * tiles_width + tile_x;
	unsigned subtileid = (subtile_y * 2 + subtile_x / 4);
	unsigned shift = (subtile_x % 4) * 4;

	return (buffer[tileid][subtileid] >> shift) & 0xF;
}

[[maybe_unused]]
static void print_tile(const tile_4bpp_t* tile) {
	for (int i = 0; i < 16; i += 2) {
		MgbaPrintf(
			MGBA_LOG_INFO,
			"    %04x, %04x",
			(*tile)[i],
			(*tile)[i+1]);
	}
}

static const struct shadow_tiles_window_allocate window_16_16 = {
	.bg = 0,
	.palette = 0,
	.x = 0,
	.y = 0,
	.width = 16,
	.height = 16,
};

static tile_4bpp_t buffer[256];
static tile_4bpp_t buffer2[256];

void test_immediate_happy(void) {
	text_print_immediate(
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {4, 4},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"A");

	static const tile_4bpp_t zero = {0};
	static const tile_4bpp_t expect_0 = {
		0x0000, 0x0000,
		0x0000, 0x0000,
		0x0000, 0x0000,
		0x0000, 0x0000,
		0x0000, 0x3444,
		0x0000, 0x1344,
		0x0000, 0x3134,
		0x0000, 0x3134,
	};
	static const tile_4bpp_t expect_1 = {
		0x0000, 0x0000,
		0x0000, 0x0000,
		0x0000, 0x0000,
		0x0000, 0x0000,
		0x0444, 0x0000,
		0x0443, 0x0000,
		0x0431, 0x0000,
		0x0431, 0x0000,
	};
	static const tile_4bpp_t expect_2 = {
		0x0000, 0x1134,
		0x0000, 0x3313,
		0x0000, 0x4313,
		0x0000, 0x4313,
		0x0000, 0x4313,
		0x0000, 0x4434,
		0x0000, 0x4444,
		0x0000, 0x4444,
	};
	static const tile_4bpp_t expect_3 = {
		0x0431, 0x0000,
		0x0313, 0x0000,
		0x0313, 0x0000,
		0x0313, 0x0000,
		0x0313, 0x0000,
		0x0434, 0x0000,
		0x0444, 0x0000,
		0x0444, 0x0000,
	};

	TEST_ASSERT_EQUAL_UINT16_ARRAY(expect_0, buffer[0], 16);
	TEST_ASSERT_EQUAL_UINT16_ARRAY(expect_1, buffer[1], 16);
	for (unsigned i = 2; i < 16; i++) TEST_ASSERT_EQUAL_UINT16_ARRAY(zero, buffer[i], 16);
	TEST_ASSERT_EQUAL_UINT16_ARRAY(expect_2, buffer[16], 16);
	TEST_ASSERT_EQUAL_UINT16_ARRAY(expect_3, buffer[17], 16);
	for (unsigned i = 18; i < 256; i++) TEST_ASSERT_EQUAL_UINT16_ARRAY(zero, buffer[i], 16);
}

void test_immediate_clip_left(void) {
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {-5, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		if (pixel_x < 16 * 8 - 5) {
			TEST_ASSERT_EQUAL_UNSIGNED(
				get_pixel_at(buffer2, window_16_16.width, pixel_x + 5, pixel_y),
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		} else {
			TEST_ASSERT_EQUAL_UNSIGNED(
				0,
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		}
	}
}

void test_immediate_clip_up(void) {
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, -5},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		if (pixel_y < 16 * 8 - 5) {
			TEST_ASSERT_EQUAL_UNSIGNED(
				get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y + 5),
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		} else {
			TEST_ASSERT_EQUAL_UNSIGNED(
				0,
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		}
	}
}

void test_immediate_clip_right(void) {
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {16*8-17, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		if (pixel_x < 16*8-17) {
			TEST_ASSERT_EQUAL_UNSIGNED(
				0,
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		} else {
			TEST_ASSERT_EQUAL_UNSIGNED(
				get_pixel_at(buffer2, window_16_16.width, pixel_x - (16*8-17), pixel_y),
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		}
	}
}

void test_immediate_clip_down(void) {
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 16*8-5},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		if (pixel_y < 16 * 8 - 5) {
			TEST_ASSERT_EQUAL_UNSIGNED(
				0,
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		} else {
			TEST_ASSERT_EQUAL_UNSIGNED(
				get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y - 16 * 8 + 5),
				get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
		}
	}
}

void test_immediate_newline(void) {
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {4, 4},
		(coord16_t) {2, 2},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {4, 4 + 2 + bitmapfont.glyph_height},
		(coord16_t) {2, 2},
		(font_colors_t) {4,1,2,3, true},
		"DEF");
	text_print_immediate(
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {4, 4},
		(coord16_t) {2, 2},
		(font_colors_t) {4,1,2,3, true},
		"ABC\nDEF");

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_happy(void) {
	struct text_print_step_state state;

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		TEXTPRINTOVERFLOW_CLIP,
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"A");
	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_CONTINUE,
		text_print_step(&state));

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {7, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"B");
	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_CONTINUE,
		text_print_step(&state));

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}


	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {13, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"C");
	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_CONTINUE,
		text_print_step(&state));

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}

	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_STOP,
		text_print_step(&state));
}

void test_stepped_clip_left(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {-4, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {-4, 0},
		(coord16_t) {0, 0},
		TEXTPRINTOVERFLOW_CLIP,
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_wraparound_left(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {-10, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {8*16-10, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {-10, 0},
		(coord16_t) {0, 0},
		(text_print_overflow_t) {TEXTPRINTOVERFLOWX_WRAPAROUND, TEXTPRINTOVERFLOWY_CLIP},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_clip_right(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {8*16-10, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {8*16-10, 0},
		(coord16_t) {0, 0},
		TEXTPRINTOVERFLOW_CLIP,
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_wraparound_right(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {-10, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {8*16-10, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {8*16-10, 0},
		(coord16_t) {0, 0},
		(text_print_overflow_t) {TEXTPRINTOVERFLOWX_WRAPAROUND, TEXTPRINTOVERFLOWY_CLIP},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_clip_up(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, -4},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, -4},
		(coord16_t) {0, 0},
		TEXTPRINTOVERFLOW_CLIP,
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_wraparound_up(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, -4},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 8*16-4},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, -4},
		(coord16_t) {0, 0},
		(text_print_overflow_t) {TEXTPRINTOVERFLOWX_CLIP, TEXTPRINTOVERFLOWY_WRAPAROUND},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_clip_down(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0,8*16-4},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0,8*16-4},
		(coord16_t) {0, 0},
		TEXTPRINTOVERFLOW_CLIP,
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_wraparound_down(void) {
	struct text_print_step_state state;

	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, -4},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");
	text_print_immediate(
		buffer2,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 8*16-4},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	text_print_step_init(
		&state,
		buffer,
		&window_16_16,
		&bitmapfont,
		(coord16_t) {0, 8*16-4},
		(coord16_t) {0, 0},
		(text_print_overflow_t) {TEXTPRINTOVERFLOWX_CLIP, TEXTPRINTOVERFLOWY_WRAPAROUND},
		(font_colors_t) {4,1,2,3, true},
		"ABC");

	while (TEXT_PRINT_STEP_STOP != text_print_step(&state)) {}

	for (int pixel_y = 0; pixel_y < 16 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_16.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_16.width, pixel_x, pixel_y));
	}
}

void test_stepped_scroll(void) {
	struct text_print_step_state state;

	static const struct shadow_tiles_window_allocate window_16_2 = {
		.bg = 0,
		.palette = 0,
		.x = 0,
		.y = 0,
		.width = 16,
		.height = 2,
	};


	text_print_step_init(
		&state,
		buffer,
		&window_16_2,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(text_print_overflow_t) {TEXTPRINTOVERFLOWX_CLIP, TEXTPRINTOVERFLOWY_SCROLL},
		(font_colors_t) {4,1,2,3, true},
		"A\nB");

	for (int pixel_y = 0; pixel_y < 16 * 2; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_2.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_2.width, pixel_x, pixel_y));
	}

	text_print_immediate(
		buffer2,
		&window_16_2,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"A");
	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_CONTINUE,
		text_print_step(&state));

	for (int pixel_y = 0; pixel_y < 2 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_2.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_2.width, pixel_x, pixel_y));
	}

	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_WAIT,
		text_print_step(&state));

	for (int pixel_y = 0; pixel_y < 2 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_2.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_2.width, pixel_x, pixel_y));
	}

	for (int i = 1; i < 13; i++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			TEXT_PRINT_STEP_CONTINUE,
			text_print_step(&state));

		for (int pixel_y = 0; pixel_y < 2 * 8; pixel_y++)
		for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
			TEST_ASSERT_EQUAL_UNSIGNED(
				get_pixel_at(buffer2, window_16_2.width, pixel_x, pixel_y + i),
				get_pixel_at(buffer, window_16_2.width, pixel_x, pixel_y));
		}
	}

	memset(buffer2, 0, sizeof(buffer2));
	text_print_immediate(
		buffer2,
		&window_16_2,
		&bitmapfont,
		(coord16_t) {0, 0},
		(coord16_t) {0, 0},
		(font_colors_t) {4,1,2,3, true},
		"B");
	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_CONTINUE,
		text_print_step(&state));

	for (int pixel_y = 0; pixel_y < 2 * 8; pixel_y++)
	for (int pixel_x = 0; pixel_x < 16 * 8; pixel_x++) {
		TEST_ASSERT_EQUAL_UNSIGNED(
			get_pixel_at(buffer2, window_16_2.width, pixel_x, pixel_y),
			get_pixel_at(buffer, window_16_2.width, pixel_x, pixel_y));
	}

	TEST_ASSERT_EQUAL_UNSIGNED(
		TEXT_PRINT_STEP_STOP,
		text_print_step(&state));
}


__attribute__((noinline))
void setUp(void){
	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));
}
void tearDown(void){}

int main() {
	total = 0;
	failed = 0;

	MgbaOpen();

	RUN_TEST(test_immediate_happy);
	RUN_TEST(test_immediate_clip_left);
	RUN_TEST(test_immediate_clip_up);
	RUN_TEST(test_immediate_clip_right);
	RUN_TEST(test_immediate_clip_down);
	RUN_TEST(test_immediate_newline);
	RUN_TEST(test_stepped_happy);
	RUN_TEST(test_stepped_clip_left);
	RUN_TEST(test_stepped_wraparound_left);
	RUN_TEST(test_stepped_clip_right);
	RUN_TEST(test_stepped_wraparound_right);
	RUN_TEST(test_stepped_clip_up);
	RUN_TEST(test_stepped_wraparound_up);
	RUN_TEST(test_stepped_clip_down);
	RUN_TEST(test_stepped_wraparound_down);
	RUN_TEST(test_stepped_scroll);



	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return 0 != failed;
}
