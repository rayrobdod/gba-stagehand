#include "scene/text_print_profile.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/palette.h"
#include "gba/vram.h"
#include "management/isr.h"
#include "graphics.h"
#include "main.h"
#include "scene/main_menu.h"
#include "text_printer.h"

_Static_assert(sizeof(uint16_t) == sizeof(bg_tile_t));
union bg_tile_2_uint {
	bg_tile_t tile;
	uint16_t uint;
};

static const struct shadow_tiles_window_allocate whole_screen_window = {
	.bg = 0,
	.palette = 0,
	.x = 0,
	.y = 0,
	.width = 32,
	.height = 32,
};

static const uint32_t zero_uint32 = 0;

static const char lorem_ipsum[] =
	"Lorem ipsum dolor sit amet, consectetur\n"
	"adipiscing elit, sed do eiusmod tempor\n"
	"incididunt ut labore et dolore magna aliqua.\n"
	"Ut enim ad minim veniam, quis nostrud\n"
	"exercitation ullamco laboris nisi ut aliquip\n"
	"ex ea commodo consequat. Duis aute irure\n"
	"dolor in reprehenderit in voluptate velit\n"
	"esse cillum dolore eu fugiat nulla pariatur.\0\n"
	"Excepteur sint occaecat cupidatat non\n"
	"proident, sunt in culpa qui officia deserunt\n"
	"mollit anim id est laborum.";

static void profile_start() {
	reg_timer[2].counter = 0;
	reg_timer[3].counter = 0;
	reg_timer[2].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {
		.timer_enable = true,
		.cascade = true,
		0
	};
	reg_timer[2].control = (timer_control_t) {
		.timer_enable = true,
		0
	};
}

static uint32_t profile_stop() {
	reg_timer[2].control = (timer_control_t) {0};
	reg_timer[3].control = (timer_control_t) {0};
	return (reg_timer[3].counter << 16) | (reg_timer[2].counter);
}

void MainCB_textPrintProfile_init(void) {
	VBlankIntrWait();
	reg_lcd.DISPCNT = (dispcnt_t){0};

	for (uint16_t i = 0; i < 32 * 32; i++) {
		vram.screenblock[31][i] = (bg_tile_t) {.tile = i, .hflip = false, .vflip = false, .palette = 0};
	}

	background_palette[0][0] = rgb(0,16,31);
	background_palette[0][1] = rgb(31,31,31);
	background_palette[0][2] = rgb(16,16,16);
	background_palette[0][3] = rgb(0,0,0);
	background_palette[0][4] = rgb(0,31,0);
	background_palette[0][5] = rgb(31,16,16);

	CpuFastSet(
		&zero_uint32,
		vram.bg_charblock[0],
		(struct CpuFastSet) {
			.word_count = 2 * sizeof(charblock_t) / sizeof(uint32_t),
			.mode = CPU_SET_FILL
		});


	profile_start();

	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {4, 4},
		(coord16_t) {-1, 0},
		(font_colors_t) {4,1,2,3, false},
		lorem_ipsum);

	uint32_t time = profile_stop();

	char buffer[16];
	unsigned start_x;

	const unsigned LABEL_X = 20;
	const unsigned CYCLE_X = 140;
	const unsigned FRAME_X = 220;
	const unsigned CHAR_Y = (8 * 20) - 4 - 12;
	const unsigned TOTAL_Y = CHAR_Y - 12;
	const unsigned LABEL_Y = TOTAL_Y - 12;
	const unsigned CYCLES_PER_FRAME = 280896;

	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {LABEL_X, TOTAL_Y},
		(coord16_t) {1,1},
		(font_colors_t) {4,1,2,3, true},
		"Total:");
	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {LABEL_X, CHAR_Y},
		(coord16_t) {1,1},
		(font_colors_t) {4,1,2,3, true},
		"Per Char:");

	start_x = text_width(&bitmapfont, (coord16_t) {1,1}, "Cycles");
	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {CYCLE_X - start_x, LABEL_Y},
		(coord16_t) {1,1},
		(font_colors_t) {0,5,2,3, true},
		"Cycles");
	start_x = text_width(&bitmapfont, (coord16_t) {1,1}, "Frames");
	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {FRAME_X - start_x, LABEL_Y},
		(coord16_t) {1,1},
		(font_colors_t) {0,5,2,3, true},
		"Frames");

	snprintf(buffer, 32, "%ld", time);
	start_x = text_width(&bitmapfont, (coord16_t) {1,1}, buffer);
	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {CYCLE_X - start_x, TOTAL_Y},
		(coord16_t) {1,1},
		(font_colors_t) {0,1,2,3, true},
		buffer);

	snprintf(buffer, 32, "%ld.%03ld", time / CYCLES_PER_FRAME, ((time * 1000) / CYCLES_PER_FRAME) % 1000);
	start_x = text_width(&bitmapfont, (coord16_t) {1,1}, buffer);
	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {FRAME_X - start_x, TOTAL_Y},
		(coord16_t) {1,1},
		(font_colors_t) {0,1,2,3, true},
		buffer);

	if (strlen(lorem_ipsum)) {
		snprintf(buffer, 32, "%ld", time / strlen(lorem_ipsum));
		start_x = text_width(&bitmapfont, (coord16_t) {1,1}, buffer);
		text_print_immediate(
			vram.bg_charblock[0],
			&whole_screen_window,
			&bitmapfont,
			(coord16_t) {CYCLE_X - start_x, CHAR_Y},
			(coord16_t) {1,1},
			(font_colors_t) {0,1,2,3, true},
			buffer);



		snprintf(buffer, 32, "%ld.%03ld", time / CYCLES_PER_FRAME / strlen(lorem_ipsum), (((time / strlen(lorem_ipsum)) * 1000) / CYCLES_PER_FRAME) % 1000);
		start_x = text_width(&bitmapfont, (coord16_t) {1,1}, buffer);
		text_print_immediate(
			vram.bg_charblock[0],
			&whole_screen_window,
			&bitmapfont,
			(coord16_t) {FRAME_X - start_x, CHAR_Y},
			(coord16_t) {1,1},
			(font_colors_t) {0,1,2,3, true},
			buffer);
	}



	VBlankIntrWait();
	reg_lcd.BGCNT[1] = (bgcnt_t){
		.charblock = 0,
		.screenblock = 31,
		.size = BGCNT_SIZE_TEXT_SMALL,
	};

	reg_lcd.DISPCNT = (dispcnt_t){
		.mode = 0,
		.enable_bg1 = true,
	};


	reg_keypad.KEYCNT = (keypad_control_t){
		.b = true,
		.condition = KEYPAD_CONDITION_AND,
	};
	isr_disable(II_VBLANK);
	isr_enable(II_KEYPAD);

	// TODO: `Stop` bios function?
	IntrWait(true, (interrupt_flag_t){.keypad = true});

	isr_disable(II_KEYPAD);
	isr_enable(II_VBLANK);

	scene_onframe_callback = &MainCB_mainMenu_init;
}
