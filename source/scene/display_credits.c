#include "scene/display_credits.h"

#include <stdint.h>
#include <stdlib.h>
#include "gba/bios.h"
#include "gba/palette.h"
#include "gba/screen.h"
#include "management/keyinput.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "utils/one_transparent_tileset.h"
#include "graphics.h"
#include "resource_credits.h"
#include "main.h"
#include "mgba.h"
#include "text_printer.h"

static void MainCB_credits_clear(void);
static void MainCB_credits_print(void);
static void MainCB_credits_idle(void);
static void MainCB_credits_clean(void);

enum {
	TITLE_WINDOW_WIDTH = 26,
	TITLE_WINDOW_HEIGHT = 2,
	URL_WINDOW_WIDTH = 28,
	URL_WINDOW_HEIGHT = 6,
	AUTHOR_WINDOW_WIDTH = 26,
	AUTHOR_WINDOW_HEIGHT = 2,
	AUTHORURL_WINDOW_WIDTH = 28,
	AUTHORURL_WINDOW_HEIGHT = 2,
	LICENSE_WINDOW_WIDTH = 26,
	LICENSE_WINDOW_HEIGHT = 2,
};

enum {
	TEXT_PAL = 2,
};

__attribute__((section(".sbss")))
static struct {
	const struct resource_credit* current;
	bg_tile_t zero_tile_ref;
	window_id_t title_window_id;
	window_id_t url_window_id;
	window_id_t author_window_id;
	window_id_t authorurl_window_id;
	window_id_t license_window_id;
	struct {
		tile_4bpp_t title[TITLE_WINDOW_WIDTH * TITLE_WINDOW_HEIGHT] __attribute__((aligned(4)));
		tile_4bpp_t url[URL_WINDOW_WIDTH * URL_WINDOW_HEIGHT] __attribute__((aligned(4)));
		tile_4bpp_t author[AUTHOR_WINDOW_WIDTH * AUTHOR_WINDOW_HEIGHT] __attribute__((aligned(4)));
		tile_4bpp_t authorurl[AUTHORURL_WINDOW_WIDTH * AUTHORURL_WINDOW_HEIGHT] __attribute__((aligned(4)));
		tile_4bpp_t license[LICENSE_WINDOW_WIDTH * LICENSE_WINDOW_HEIGHT] __attribute__((aligned(4)));
	} window_shadow_tiles;
}* view_model = NULL;

static const palette16_t text_pal = {
	{10, 10, 10},
	{30, 10, 10},
	{10, 30, 10},
	{30, 30, 0},
	{10, 10, 30},
	{30, 10, 30},
	{10, 30, 30},
	{31, 31, 31},
	{0, 0, 0},
	{20, 0, 0},
	{0, 20, 0},
	{15, 15, 0},
	{0, 0, 20},
	{20, 0, 20},
	{0, 20, 20},
	{20, 20, 20},
};

static const struct shadow_vram_init vram_init = (struct shadow_vram_init) {
	.enable_bg = {false, false, false, true},
	.enable_obj = false,
	.bgcnt = {
		[0] = {
			.priority = 3,
			.charblock = 0,
			.screenblock = 31,
		},
		[3] = {
			.priority = 0,
			.charblock = 0,
			.screenblock = 30,
		}
	}
};

static const struct shadow_tiles_window_allocate title_window_template = {
	.bg = 3,
	.palette = TEXT_PAL,
	.x = 2,
	.y = 2,
	.width = TITLE_WINDOW_WIDTH,
	.height = TITLE_WINDOW_HEIGHT,
};

static const struct shadow_tiles_window_allocate author_window_template = {
	.bg = 3,
	.palette = TEXT_PAL,
	.x = 2,
	.y = 5,
	.width = AUTHOR_WINDOW_WIDTH,
	.height = AUTHOR_WINDOW_HEIGHT,
};
static const struct shadow_tiles_window_allocate authorurl_window_template = {
	.bg = 3,
	.palette = TEXT_PAL,
	.x = 1,
	.y = 7,
	.width = AUTHORURL_WINDOW_WIDTH,
	.height = AUTHORURL_WINDOW_HEIGHT,
};
static const struct shadow_tiles_window_allocate url_window_template = {
	.bg = 3,
	.palette = TEXT_PAL,
	.x = 1,
	.y = 10,
	.width = URL_WINDOW_WIDTH,
	.height = URL_WINDOW_HEIGHT,
};
static const struct shadow_tiles_window_allocate license_window_template = {
	.bg = 3,
	.palette = TEXT_PAL,
	.x = 2,
	.y = 16,
	.width = LICENSE_WINDOW_WIDTH,
	.height = LICENSE_WINDOW_HEIGHT,
};

void MainCB_credits_init(void) {
	view_model = calloc(sizeof(view_model[0]), 1);

	shadow_vram_init(&vram_init);
	hw_palette.background._4[0][0] = rgb(31,16,16);

	view_model->current = resource_credits;
	view_model->zero_tile_ref = (bg_tile_t) {.tile = shadow_tiles_load_tileset(&one_transparent_tileset, (struct shadow_tiles_load_tileset) {0})};

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = view_model->zero_tile_ref,
			.to_block = vram_init.bgcnt[3].screenblock,
			.to_tile = 0,
			.count = 32 * 32,
		},
	});

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_PALETTES,
		.palettes = {
			.from = &text_pal,
			.to_palette = TEXT_PAL,
			.count = 1,
		},
	});

	view_model->title_window_id = shadow_tiles_window_allocate(&title_window_template);
	shadow_tiles_window_queue_map(view_model->title_window_id);
	view_model->author_window_id = shadow_tiles_window_allocate(&author_window_template);
	shadow_tiles_window_queue_map(view_model->author_window_id);
	view_model->authorurl_window_id = shadow_tiles_window_allocate(&authorurl_window_template);
	shadow_tiles_window_queue_map(view_model->authorurl_window_id);
	view_model->url_window_id = shadow_tiles_window_allocate(&url_window_template);
	shadow_tiles_window_queue_map(view_model->url_window_id);
	view_model->license_window_id = shadow_tiles_window_allocate(&license_window_template);
	shadow_tiles_window_queue_map(view_model->license_window_id);

	MainCB_credits_clear();
}

static void MainCB_credits_clear(void) {
	uint32_t z = 0;
	_Static_assert(sizeof(view_model->window_shadow_tiles) % 32 == 0);

	CpuFastSet(
		&z,
		&view_model->window_shadow_tiles,
		(struct CpuFastSet){
			.word_count = (sizeof(view_model->window_shadow_tiles) / sizeof(uint32_t)),
			.mode = CPU_SET_FILL,
		});

	shadow_tiles_window_queue_tiles(view_model->title_window_id, view_model->window_shadow_tiles.title);
	shadow_tiles_window_queue_tiles(view_model->author_window_id, view_model->window_shadow_tiles.author);
	shadow_tiles_window_queue_tiles(view_model->authorurl_window_id, view_model->window_shadow_tiles.authorurl);
	shadow_tiles_window_queue_tiles(view_model->url_window_id, view_model->window_shadow_tiles.url);
	shadow_tiles_window_queue_tiles(view_model->license_window_id, view_model->window_shadow_tiles.license);

	scene_onframe_callback = &MainCB_credits_print;
}

static void MainCB_credits_print(void) {
	text_print_immediate(
		view_model->window_shadow_tiles.title,
		&title_window_template,
		&bitmapfont,
		(coord16_t) {1, 1},
		(coord16_t) {1, 1},
		(font_colors_t) {0, 7, 15, 8, false},
		view_model->current->title);

	text_print_immediate(
		view_model->window_shadow_tiles.author,
		&author_window_template,
		&bitmapfont,
		(coord16_t) {1, 1},
		(coord16_t) {1, 1},
		(font_colors_t) {0, 7, 15, 8, false},
		view_model->current->author);

	text_print_immediate(
		view_model->window_shadow_tiles.authorurl,
		&authorurl_window_template,
		&bitmapfont,
		(coord16_t) {1, 1},
		(coord16_t) {-1, 0},
		(font_colors_t) {0, 12, 15, 0, false},
		view_model->current->author_url);

	text_print_immediate(
		view_model->window_shadow_tiles.license,
		&license_window_template,
		&bitmapfont,
		(coord16_t) {1, 1},
		(coord16_t) {1, 1},
		(font_colors_t) {0, 7, 15, 8, false},
		view_model->current->licensed_under);

	{
		struct text_print_step_state url_print_step_state;
		enum text_print_step_retval url_print_step_retval = TEXT_PRINT_STEP_CONTINUE;
		text_print_step_init(
			&url_print_step_state,
			view_model->window_shadow_tiles.url,
			&url_window_template,
			&bitmapfont,
			(coord16_t) {1, 1},
			(coord16_t) {-1, 0},
			(font_colors_t) {0, 12, 15, 0, false},
			view_model->current->retrieved_from);

		const int max_x = TILE_PIXEL_SIDE * URL_WINDOW_WIDTH - 1;

		while (TEXT_PRINT_STEP_STOP != url_print_step_retval) {
			url_print_step_retval = text_print_step(&url_print_step_state);

			char next_c = url_print_step_state.message[0];
			if (' ' <= next_c && next_c < ' ' + url_print_step_state.font->glyph_count) {
				uint16_t next_width = url_print_step_state.font->glyphs[next_c - ' '].width;

				if (url_print_step_state.current_point.x + next_width > max_x) {
					url_print_step_state.current_point.x = url_print_step_state.start_point.x;
					url_print_step_state.current_point.y += url_print_step_state.font->glyph_height + url_print_step_state.kerning.y;
				}
			}
		}
	}

	shadow_tiles_window_queue_tiles(view_model->title_window_id, view_model->window_shadow_tiles.title);
	shadow_tiles_window_queue_tiles(view_model->author_window_id, view_model->window_shadow_tiles.author);
	shadow_tiles_window_queue_tiles(view_model->authorurl_window_id, view_model->window_shadow_tiles.authorurl);
	shadow_tiles_window_queue_tiles(view_model->url_window_id, view_model->window_shadow_tiles.url);
	shadow_tiles_window_queue_tiles(view_model->license_window_id, view_model->window_shadow_tiles.license);

	scene_onframe_callback = &MainCB_credits_idle;
}

static void MainCB_credits_idle(void) {
	if (! keyinput_get_new().b) {
		scene_onframe_callback = &MainCB_credits_clean;
	}
	if (! keyinput_get_new().left && (view_model->current > resource_credits)) {
		view_model->current--;
		MainCB_credits_clear();
	}
	if (! keyinput_get_new().right && ((view_model->current + 1) < resource_credits_end)) {
		view_model->current++;
		MainCB_credits_clear();
	}
}

static void MainCB_credits_clean(void) {
	if (view_model) {
		free(view_model);
		view_model = NULL;
	}

	scene_onframe_callback = &MainCB_mainMenu_init;
}
