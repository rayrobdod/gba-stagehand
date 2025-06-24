#include "scene/text_print_step.h"

#include <stdlib.h>
#include "management/keyinput.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "utils/arraycount.h"
#include "graphics.h"
#include "graphics_types.h"
#include "main.h"

static void MainCB_textPrintStep_main(void);
static void MainCB_textPrintStep_cleanup(void);

_Static_assert(sizeof(uint16_t) == sizeof(bg_tile_t));
union bg_tile_2_uint {
	bg_tile_t tile;
	uint16_t uint;
};

__attribute__((section(".sbss")))
static struct {
	bg_tile_t zero_tile_ref;
	uint32_t border_tile_id;
	window_id_t dialog_window_id;

}* view_model = NULL;

static const char zero_tile[sizeof(tile_4bpp_t) + 4] = {0, sizeof(tile_4bpp_t), 0};
static const struct tileset one_zero_tileset = {
	.palette = NULL,
	.tileset = zero_tile,
	.tileset_count = 1,
};

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


static const unsigned BORDER_PALETTE_NO = 1;
static const unsigned TILEMAP_BUFFER_COUNT = 32 * 20;

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

	#define TILE(index) ((bg_tile_t) {.tile = view_model->border_tile_id + index, .palette = BORDER_PALETTE_NO})

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

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FREE,
		.map_free = {
			.from = map,
			.to_block = 31,
			.to_tile = 0,
			.count = TILEMAP_BUFFER_COUNT,
		},
	});
}

void MainCB_textPrintStep_init(void) {
	view_model = calloc(sizeof(view_model), 1);
	shadow_vram_init(&text_print_shadow_vram_init);

	background_palette[0][0] = rgb(0,16,31);

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_PALETTES,
		.palettes = {
			.from = dialog_box.palette,
			.to_palette = BORDER_PALETTE_NO,
			.count = 1,
		},
	});

	view_model->zero_tile_ref = (bg_tile_t) {.tile = shadow_tiles_load_tileset(&one_zero_tileset, (struct shadow_tiles_load_tileset) {0})};
	view_model->border_tile_id = shadow_tiles_load_tileset(&dialog_box, (struct shadow_tiles_load_tileset) {0});
	view_model->dialog_window_id = shadow_tiles_window_allocate(&dialog_window_template);

	gen_window_border();

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = view_model->zero_tile_ref,
			.to_block = 30,
			.to_tile = 0,
			.count = TILEMAP_BUFFER_COUNT,
		},
	});

	scene_onframe_callback = &MainCB_textPrintStep_main;
}

static void MainCB_textPrintStep_main(void) {

}

static void MainCB_textPrintStep_cleanup(void) {
	free(view_model);
	view_model = NULL;

	scene_onframe_callback = &MainCB_mainMenu_init;
}
