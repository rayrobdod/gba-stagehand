#include "scene/text_print_step.h"

#include <stdlib.h>
#include "management/keyinput.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "transition/palette_fade.h"
#include "utils/ansi_text_palette.h"
#include "utils/arraycount.h"
#include "utils/one_transparent_tileset.h"
#include "graphics.h"
#include "main.h"
#include "mgba.h"
#include "options.h"
#include "text_printer.h"

static void MainCB_textPrintStep(void);
static union palette512 InitFadeIn_mainMenu(void);
static void CleanupCB_textPrintStep(void);

_Static_assert(sizeof(uint16_t) == sizeof(bg_tile_t));
union bg_tile_2_uint {
	bg_tile_t tile;
	uint16_t uint;
};

__attribute__((section(".sbss")))
static struct {
	bg_tile_t zero_tile_ref;
	shadow_tiles_load_tileset_retval_t border_tile_ids;
	window_id_t dialog_window_id;
	struct text_print_step_state printer_state;
	enum text_print_step_retval printer_retval;
	tile_4bpp_t* dialog_window_shadow_tiles;
}* view_model = NULL;

static const char lorem_ipsum[] =
	"Lorem ipsum dolor sit amet,\n"
	"consectetur adipiscing elit, sed\n"
	"do eiusmod tempor incididunt ut\n"
	"labore et dolore magna aliqua.\f"
	"Ut enim ad minim veniam, quis\n"
	"nostrud exercitation ullamco\n"
	"laboris nisi ut aliquip ex ea\n"
	"commodo consequat. Duis aute\n"
	"irure dolor in reprehenderit in\n"
	"voluptate velit esse cillum\n"
	"dolore eu fugiat nulla pariatur.\f"
	"Excepteur sint occaecat\n"
	"cupidatat non proident, sunt\n"
	"in culpa qui officia deserunt\n"
	"mollit anim id est laborum.";

static const struct shadow_vram_init text_print_shadow_vram_init = {
	.enable_bg = {true, false, false, true},
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

static const struct shadow_tiles_window_allocate dialog_window_template = {
	.bg = 3,
	.palette = 15,
	.x = (30 - 26) / 2,
	.y = (20 - 4) / 2,
	.width = 26,
	.height = 4,
};


static const unsigned TEXT_PALETTE_NO = 15;
static const unsigned TILEMAP_BUFFER_COUNT = 32 * 20;

static const struct transitionSourceCallbacks transitionSourceCbs_textPrintStep = {
	.fadeOut = NULL,
	.cleanup = CleanupCB_textPrintStep,
};
const struct transitionTargetCallbacks transitionTargetCbs_textPrintStep = {
	.initFadeOut = NULL,
	.fadeOut = NULL,
	.initFadeIn = InitFadeIn_mainMenu,
	.fadeIn = NULL,
	.target = MainCB_textPrintStep,
};


static void gen_window_border(void) {
	union bg_tile_2_uint convert = {.tile = view_model->zero_tile_ref};
	uint32_t zero_tile_ref_pair = convert.uint << 16 | convert.uint;

	bg_tile_t* map = malloc(sizeof(bg_tile_t) * TILEMAP_BUFFER_COUNT);
	CpuFastSet(&zero_tile_ref_pair, map, (struct CpuFastSet) {
		.word_count = TILEMAP_BUFFER_COUNT * sizeof(bg_tile_t) / sizeof(uint32_t),
		.mode = CPU_SET_FILL
	});

	const unsigned top = dialog_window_template.y;
	const unsigned bot = top + dialog_window_template.height;
	const unsigned left = dialog_window_template.x;
	const unsigned right = left + dialog_window_template.width;

	#define TILE(index) ((bg_tile_t) {.tile = view_model->border_tile_ids.tileid + index, .palette = view_model->border_tile_ids.palid})

	map[(left - 1) + 32 * (top - 1)] = TILE(0);
	map[(right) + 32 * (top - 1)] = TILE(2);
	map[(left - 1) + 32 * (bot)] = TILE(6);
	map[(right) + 32 * (bot)] = TILE(8);
	for (unsigned x = left; x < right; x++) {
		map[x + 32 * (top - 1)] = TILE(1);
		map[x + 32 * (bot)] = TILE(7);
	}

	for (unsigned y = top; y < bot; y++) {
		map[(left - 1) + 32 * y] = TILE(3);
		map[(right) + 32 * y] = TILE(5);
		for (unsigned x = left; x < right; x++) {
			map[x + 32 * y] = TILE(4);
		}
	}

	#undef TILE

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FREE,
		.map_free = {
			.from = map,
			.to_block = 31,
			.to_tile = 0,
			.count = TILEMAP_BUFFER_COUNT,
		},
	});
}

static union palette512 InitFadeIn_mainMenu(void) {
	union palette512 palette = {0};
	view_model = calloc(sizeof(view_model[0]), 1);
	if (false) {
		MgbaPrintf(MGBA_LOG_DEBUG, "  view_model: %p, %d", view_model, sizeof(view_model[0]));
	}
	shadow_vram_init(&text_print_shadow_vram_init);

	const struct tileset* frame = options_frame_get();

	view_model->zero_tile_ref = (bg_tile_t) {.tile = shadow_tiles_load_tileset(&one_transparent_tileset, (shadow_tiles_load_tileset_args_t) {0}).tileid};
	view_model->border_tile_ids = shadow_tiles_load_tileset_no_palette_vram_op(&palette, frame, (shadow_tiles_load_tileset_args_t) {0});
	view_model->dialog_window_id = shadow_tiles_window_allocate(&dialog_window_template);

	gen_window_border();

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = view_model->zero_tile_ref,
			.to_block = 30,
			.to_tile = 0,
			.count = TILEMAP_BUFFER_COUNT,
		},
	});

	CpuFastCopy(ansi_text_palette, palette.background._4[TEXT_PALETTE_NO], sizeof(palette16_t) / sizeof(uint32_t));

	view_model->dialog_window_shadow_tiles = calloc(sizeof(tile_4bpp_t), dialog_window_template.width * dialog_window_template.height);

	text_print_step_init(
		&view_model->printer_state,
		view_model->dialog_window_shadow_tiles,
		&dialog_window_template,
		&bitmapfont,
		(coord16_t) {2,3},
		(coord16_t) {1,2},
		(font_colors_t) {0, 7, 15, 8, false},
		lorem_ipsum);

	view_model->printer_retval = TEXT_PRINT_STEP_CONTINUE;

	shadow_tiles_window_queue_tiles(view_model->dialog_window_id, view_model->dialog_window_shadow_tiles);
	shadow_tiles_window_queue_map(view_model->dialog_window_id);

	palette.background._4[0][0] = rgb(4, 18, 31);

	return palette;
}

static void MainCB_textPrintStep(void) {
	switch (view_model->printer_retval) {
	case TEXT_PRINT_STEP_CONTINUE:
		{
			view_model->printer_retval = text_print_step(&view_model->printer_state);
			shadow_tiles_window_queue_tiles(view_model->dialog_window_id, view_model->dialog_window_shadow_tiles);
		}
		break;
	case TEXT_PRINT_STEP_WAIT:
		if (! keyinput_get_new().a || ! keyinput_get_down().r) {
			view_model->printer_retval = TEXT_PRINT_STEP_CONTINUE;
			MainCB_textPrintStep();
		}
		break;
	case TEXT_PRINT_STEP_STOP:
		if (! keyinput_get_new().a) {
			StartTransition(
				&transition_paletteFade_dodgerblue,
				&transitionSourceCbs_textPrintStep,
				&transitionTargetCbs_mainMenu);
		}
		break;
	}

	if (! keyinput_get_new().b) {
		StartTransition(
			&transition_paletteFade_dodgerblue,
			&transitionSourceCbs_textPrintStep,
			&transitionTargetCbs_mainMenu);
	}
}

static void CleanupCB_textPrintStep(void) {
	if (view_model->dialog_window_shadow_tiles) {
		free(view_model->dialog_window_shadow_tiles);
		view_model->dialog_window_shadow_tiles = NULL;
	}
	if (view_model) {
		free(view_model);
		view_model = NULL;
	}
}
