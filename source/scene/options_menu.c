#include "scene/options_menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "management/keyinput.h"
#include "management/shadow_oam.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "utils/ansi_text_palette.h"
#include "utils/one_transparent_tileset.h"
#include "utils/saturating_add.h"
#include "../options.h"
#include "graphics.h"
#include "main.h"
#include "mgba.h"
#include "mix_rgb.h"
#include "text_printer.h"

static void MainCB_options_fadeout_black(void);
static void MainCB_options_fadesolid_black(void);
static void MainCB_options_fadein_black(void);
static void options_update_background(void);
static void MainCB_options_main(void);

enum {
	BACKGROUND_BG = 3,
	BACKGROUND_CHARBLOCK = 2,
	BACKGROUND_SCREENBLOCK = 27,
	BACKGROUND_PAL = 13,
	BACKGROUND_FREQUENCY = 6,

	BORDER_LEFT = 1,
	BORDER_TOP = 1,
	BORDER_RIGHT = 28,
	BORDER_BOTTOM = 19,
	BORDER_BG = 2,
	BORDER_CHARBLOCK = 2,
	BORDER_SCREENBLOCK = 28,
	BORDER_PAL = 14,

	TEXT_BG = 1,
	TEXT_CHARBLOCK = 0,
	TEXT_SCREENBLOCK = 30,
	TEXT_PAL = 15,
};

enum options_row {
	OPTIONS_ROW_FRAME,
	OPTIONS_ROW_EXIT,
};

enum options_exit_type {
	OPTIONS_EXIT_COMMIT,
	OPTIONS_EXIT_CANCEL,
};

__attribute__((section(".sbss")))
void (*fadeCb)(void) = {0};
__attribute__((section(".sbss")))
void (*ChangeScene_return)(void (*fadeCb)(void));

__attribute__((section(".sbss.viewmodel.options")))
static struct options_viewmodel {
	struct options values;
	enum options_exit_type exit_selection;
	enum options_row row;

	uint16_t text_transparent_tile;
	uint16_t border_tile;
	window_id_t title_window;
	window_id_t main_window;
	bgofs_t background_offset;
	uint16_t background_move_delay;
	[[gnu::aligned(4)]] tile_4bpp_t main_window_tiles[26 * 14];
} options_viewmodel = {0};

static const struct shadow_vram_init options_shadow_vram_init = {
	.enable_bg = {false, true, true, true},
	.enable_obj = true,
	.bgcnt = {
		[TEXT_BG] = {
			.priority = 1,
			.charblock = TEXT_CHARBLOCK,
			.screenblock = TEXT_SCREENBLOCK,
		},
		[BORDER_BG] = {
			.priority = 2,
			.charblock = BORDER_CHARBLOCK,
			.screenblock = BORDER_SCREENBLOCK,
		},
		[BACKGROUND_BG] = {
			.priority = 3,
			.charblock = BACKGROUND_CHARBLOCK,
			.screenblock = BACKGROUND_SCREENBLOCK,
		}
	}
};

static const struct shadow_tiles_window_allocate window_allocate_title = {
	.bg = TEXT_BG,
	.palette = TEXT_PAL,
	.x = 10,
	.y = 2,
	.width = 30 - 10 * 2,
	.height = 2,
};
static const struct shadow_tiles_window_allocate window_allocate_main = {
	.bg = TEXT_BG,
	.palette = TEXT_PAL,
	.x = 2,
	.y = 5,
	.width = 30 - 2 * 2,
	.height = 20 - 1 - 5,
};

static const font_colors_t TITLE_COLORS = {
	.light = ANSI_PALETTE_WHITE,
	.shadow = ANSI_PALETTE_GREY,
	.dark = ANSI_PALETTE_BLACK,
	.write_background = false,
};
static const font_colors_t NORMAL_COLORS = {
	.light = ANSI_PALETTE_BLACK,
	.shadow = ANSI_PALETTE_WHITE,
	.dark = ANSI_PALETTE_WHITE,
	.write_background = false,
};
static const font_colors_t SELECTED_COLORS = {
	.light = ANSI_PALETTE_RED,
	.shadow = 0,
	.dark = 0,
	.write_background = false,
};


static void options_draw_main_text(enum options_row row) {
	switch (row) {
	case OPTIONS_ROW_FRAME:
		text_print_immediate(
			options_viewmodel.main_window_tiles,
			&window_allocate_main,
			&bitmapfont,
			(coord16_t) {.x = 4, .y = 4},
			(coord16_t) {.x = 1, .y = 1},
			(options_viewmodel.row == OPTIONS_ROW_FRAME ? SELECTED_COLORS : NORMAL_COLORS),
			"Frame:");

		text_clear_immediate(
			options_viewmodel.main_window_tiles,
			&window_allocate_main,
			(coord16_t) {.x = 80, .y = 4},
			(coord16_t) {.x = 80 + 16, .y = 4 + bitmapfont.glyph_height},
			0);

		char index_str[4] = {0};
		snprintf(index_str, sizeof(index_str), "%d", options_viewmodel.values.frame_index);

		text_print_immediate(
			options_viewmodel.main_window_tiles,
			&window_allocate_main,
			&bitmapfont,
			(coord16_t) {.x = 80, .y = 4},
			(coord16_t) {.x = 1, .y = 1},
			(options_viewmodel.row == OPTIONS_ROW_FRAME ? SELECTED_COLORS : NORMAL_COLORS),
			index_str);
		break;

	case OPTIONS_ROW_EXIT:
		text_print_immediate(
			options_viewmodel.main_window_tiles,
			&window_allocate_main,
			&bitmapfont,
			(coord16_t) {.x = 100, .y = 100},
			(coord16_t) {.x = 1, .y = 1},
			(options_viewmodel.row == OPTIONS_ROW_EXIT && options_viewmodel.exit_selection == OPTIONS_EXIT_COMMIT ? SELECTED_COLORS : NORMAL_COLORS),
			"OK");

		text_print_immediate(
			options_viewmodel.main_window_tiles,
			&window_allocate_main,
			&bitmapfont,
			(coord16_t) {.x = 140, .y = 100},
			(coord16_t) {.x = 1, .y = 1},
			(options_viewmodel.row == OPTIONS_ROW_EXIT && options_viewmodel.exit_selection == OPTIONS_EXIT_CANCEL ? SELECTED_COLORS : NORMAL_COLORS),
			"Cancel");
		break;
	}
}

void ChangeScene_options(void (*_fadeCb)(void), void (*_ChangeScene_return)(void (*)(void))) {
	fade_to_initialize(rgb(0, 0, 0));
	fadeCb = _fadeCb;
	ChangeScene_return = _ChangeScene_return;
	scene_onframe_callback = &MainCB_options_fadeout_black;
}

static void MainCB_options_fadeout_black(void) {
	if (fade_step()) {
		scene_onframe_callback = &MainCB_options_fadesolid_black;
	} else {
		fadeCb();
	}
}

static union palette512 MainCB_options_fadesolid(void) {
	shadow_vram_init(&options_shadow_vram_init);
	shadow_oam_init();
	options_viewmodel = (struct options_viewmodel) {0};
	options_viewmodel.values = options;

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {{0,0},{0,4},{0,4},{0,0},},
		},
	});

	int background_tile = shadow_tiles_load_tileset(&diagonal_blue_background_tile, (struct shadow_tiles_load_tileset) {.bg = BACKGROUND_BG});
	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = {background_tile, false, false, BACKGROUND_PAL},
			.to_block = BACKGROUND_SCREENBLOCK,
			.to_tile = 0,
			.count = sizeof(screenblock_t) / sizeof(bg_tile_t),
		},
	});

	options_viewmodel.text_transparent_tile = shadow_tiles_load_tileset(&one_transparent_tileset, (struct shadow_tiles_load_tileset) {.bg = TEXT_BG});
	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = {options_viewmodel.text_transparent_tile},
			.to_block = TEXT_SCREENBLOCK,
			.to_tile = 0,
			.count = 2 * sizeof(screenblock_t) / sizeof(bg_tile_t),
		},
	});

	int border_transparent_tile = shadow_tiles_load_tileset(&one_transparent_tileset, (struct shadow_tiles_load_tileset) {.bg = BORDER_BG});
	border_transparent_tile = border_transparent_tile | (BORDER_PAL << 12);
	border_transparent_tile = border_transparent_tile | (border_transparent_tile << 16);
	options_viewmodel.border_tile = shadow_tiles_load_tileset(options_frame_get(), (struct shadow_tiles_load_tileset) {.bg = BORDER_BG});
	bg_tile_t* border_map = malloc(sizeof(screenblock_t));
	if (border_map) {
		CpuFastSet(
			&border_transparent_tile,
			border_map,
			(struct CpuFastSet) {
				.word_count = sizeof(screenblock_t) / sizeof(uint32_t),
				.mode = CPU_SET_FILL,
			}
		);
		border_map[BORDER_LEFT + 32 * BORDER_TOP] = (bg_tile_t) {options_viewmodel.border_tile, false, false, BORDER_PAL};
		border_map[BORDER_RIGHT + 32 * BORDER_TOP] = (bg_tile_t) {options_viewmodel.border_tile + 2, false, false, BORDER_PAL};
		border_map[BORDER_LEFT + 32 * BORDER_BOTTOM] = (bg_tile_t) {options_viewmodel.border_tile + 6, false, false, BORDER_PAL};
		border_map[BORDER_RIGHT + 32 * BORDER_BOTTOM] = (bg_tile_t) {options_viewmodel.border_tile + 8, false, false, BORDER_PAL};
		for (unsigned x = BORDER_LEFT + 1; x < BORDER_RIGHT; x++) {
			border_map[x + 32 * BORDER_TOP] = (bg_tile_t) {options_viewmodel.border_tile + 1, false, false, BORDER_PAL};
			border_map[x + 32 * BORDER_BOTTOM] = (bg_tile_t) {options_viewmodel.border_tile + 7, false, false, BORDER_PAL};
		}
		for (unsigned y = BORDER_TOP + 1; y < BORDER_BOTTOM; y++) {
			border_map[BORDER_LEFT + 32 * y] = (bg_tile_t) {options_viewmodel.border_tile + 3, false, false, BORDER_PAL};
			for (unsigned x = BORDER_LEFT + 1; x < BORDER_RIGHT; x++) {
				border_map[x + 32 * y] = (bg_tile_t) {options_viewmodel.border_tile + 4, false, false, BORDER_PAL};
			}
			border_map[BORDER_RIGHT + 32 * y] = (bg_tile_t) {options_viewmodel.border_tile + 5, false, false, BORDER_PAL};
		}

		vram_op_queue_enqueue(&(struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = border_map,
				.to_block = BORDER_SCREENBLOCK,
				.to_tile = 0,
				.count = sizeof(screenblock_t) / sizeof(bg_tile_t),
			},
		});
	}

	options_viewmodel.title_window = shadow_tiles_window_allocate(&window_allocate_title);
	shadow_tiles_window_queue_map(options_viewmodel.title_window);
	size_t title_buffer_size = sizeof(tile_4bpp_t) * window_allocate_title.width * window_allocate_title.height;
	tile_4bpp_t* title_buffer = malloc(title_buffer_size);
	if (title_buffer) {
		coord16_t kerning = {.x = 1, .y = 1};
		const char* message = "OPTIONS";
		uint32_t zero = 0;
		CpuFastSet(
			&zero,
			title_buffer,
			(struct CpuFastSet){
				.word_count = title_buffer_size / sizeof(uint32_t),
				.mode = CPU_SET_FILL,
			});

		text_print_immediate(
			title_buffer,
			&window_allocate_title,
			&bitmapfont,
			(coord16_t) {
				.x = ((8 * window_allocate_title.width) - text_width(&bitmapfont, kerning, message)) / 2,
				.y = ((8 * window_allocate_title.height) - bitmapfont.glyph_height) / 2 + 1,
			},
			kerning,
			TITLE_COLORS,
			message);
		shadow_tiles_window_queue_tiles_free(
			options_viewmodel.title_window, title_buffer);
	}

	options_viewmodel.main_window = shadow_tiles_window_allocate(&window_allocate_main);
	shadow_tiles_window_queue_map(options_viewmodel.main_window);
	{
		for (enum options_row i = 0; i <= OPTIONS_ROW_EXIT; i++) {
			options_draw_main_text(i);
		}

		shadow_tiles_window_queue_tiles(
			options_viewmodel.main_window, options_viewmodel.main_window_tiles);
	}

	union palette512 final_palette = {0};
	memcpy(
		final_palette.background._4[BACKGROUND_PAL],
		diagonal_blue_background_tile.palette, sizeof(palette16_t));
	memcpy(
		final_palette.background._4[BORDER_PAL],
		options_frame_get()->palette, sizeof(palette16_t));
	memcpy(
		final_palette.background._4[TEXT_PAL],
		ansi_text_palette, sizeof(palette16_t));
	return final_palette;
}

static void MainCB_options_fadesolid_black(void) {
	union palette512 final_palette = MainCB_options_fadesolid();
	fade_from_initialize(final_palette.all, 512);
	scene_onframe_callback = &MainCB_options_fadein_black;
}

static void MainCB_options_fadein_black(void) {
	if (fade_step()) {
		scene_onframe_callback = &MainCB_options_main;
	}
	options_update_background();
}

static void options_update_background(void) {
	if (0 == options_viewmodel.background_move_delay) {
		options_viewmodel.background_move_delay = BACKGROUND_FREQUENCY;
		options_viewmodel.background_offset.v -= 1;

		vram_op_queue_enqueue(&(struct vram_op) {
			.type = VRAM_QUEUE_OP_UINT16,
			.uint16 = {
				.value = options_viewmodel.background_offset.v,
				.to = &reg_lcd.BGOFS[BACKGROUND_BG].v
			},
		});
	} else {
		options_viewmodel.background_move_delay -= 1;
	}
}

static void options_process_input(void) {
	const int row_delta = keyinput_vertical_new();
	if (0 != row_delta) {
		enum options_row before_row = options_viewmodel.row;
		options_viewmodel.row = saturating_add(options_viewmodel.row, 0, OPTIONS_ROW_EXIT, row_delta);
		options_draw_main_text(before_row);
		options_draw_main_text(options_viewmodel.row);

		shadow_tiles_window_queue_tiles(
			options_viewmodel.main_window, options_viewmodel.main_window_tiles);
	}
	const int col_delta = keyinput_horizontal_new();
	if (0 != col_delta) {
		switch (options_viewmodel.row) {
		case OPTIONS_ROW_FRAME:
			options_viewmodel.values.frame_index = saturating_add(options_viewmodel.values.frame_index, 0, OPTIONS_FRAMES_COUNT - 1, col_delta);

			const struct tileset* new_border = options_frame_get_n(options_viewmodel.values.frame_index);

			vram_op_queue_enqueue(&(struct vram_op) {
				.type = VRAM_QUEUE_OP_BG_TILES_COMPRESSED,
				.tiles_compressed = {
					.from = new_border->tileset,
					.to_block = BORDER_CHARBLOCK,
					.to_tile = options_viewmodel.border_tile,
				},
			});
			vram_op_queue_enqueue(&(struct vram_op) {
				.type = VRAM_QUEUE_OP_BG_PALETTES,
				.palettes = {
					.from = new_border->palette,
					.to_palette = BORDER_PAL,
					.count = 1,
				},
			});

			break;
		case OPTIONS_ROW_EXIT:
			options_viewmodel.exit_selection = saturating_add(options_viewmodel.exit_selection, 0, 1, col_delta);
			break;
		}
		options_draw_main_text(options_viewmodel.row);
		shadow_tiles_window_queue_tiles(
			options_viewmodel.main_window, options_viewmodel.main_window_tiles);
	}
	keypad_t new_inputs = keyinput_get_new();
	if (! new_inputs.a) {
		switch (options_viewmodel.row) {
		case OPTIONS_ROW_FRAME:
			break;
		case OPTIONS_ROW_EXIT:
			if (options_viewmodel.exit_selection == OPTIONS_EXIT_COMMIT)
				options = options_viewmodel.values;

			ChangeScene_return(&options_update_background);
			break;
		}
	}
	else if (! new_inputs.b) {
		ChangeScene_return(&options_update_background);
	}
	else if (! new_inputs.start) {
		options = options_viewmodel.values;
		ChangeScene_return(&options_update_background);
	}
}

void MainCB_options_main(void) {
	options_update_background();
	options_process_input();
}
