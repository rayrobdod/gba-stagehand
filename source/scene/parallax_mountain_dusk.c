#include "scene/parallax_mountain_dusk.h"

#include <stdlib.h>
#include "decompress/by_header.h"
#include "decompress/type.h"
#include "gba/screen.h"
#include "gba/vram.h"
#include "management/keyinput.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "graphics.h"
#include "graphics_types.h"
#include "main.h"
#include "mgba.h"
#include "mix_rgb.h"

static void MainCB_parallaxMountainDusk_initFadeOut(void);
static void MainCB_parallaxMountainDusk_initFadeSolid(void);
static void MainCB_parallaxMountainDusk_initFadeIn(void);
void MainCB_parallaxMountainDusk_main(void);
static void MainCB_parallaxMountainDusk_cleanup(void);

struct parallax_layer {
	uint16_t hardware_offset;
	uint16_t next_hardware_column;
	uint16_t next_source_column;
};

__attribute__((section(".sbss")))
static struct {
	uint16_t timer;
	struct parallax_layer mountain_far;
	struct parallax_layer mountains;
	struct parallax_layer trees;
}* view_model = NULL;

__attribute__((section(".sbss")))
static void (*fadeCb)(void) = {0};

enum {
	MY_CHARBLOCK = 0,
	SKY_SCREENBLOCK = 24,
	MOUNTAIN_FAR_SCREENBLOCK = 25,
	MOUNTAINS_SCREENBLOCK = 26,
	TREES_SCREENBLOCK = 27,

	MOUNTAINFAR_INITIAL_OFFSET = 7,
	MOUNTAINS_INITIAL_OFFSET = -3,
};

static const bgcnt_t my_bgcnts[4] = {
	[0] = {
		.priority = 3,
		.charblock = MY_CHARBLOCK,
		.screenblock = SKY_SCREENBLOCK,
	},
	[1] = {
		.priority = 2,
		.charblock = MY_CHARBLOCK,
		.screenblock = MOUNTAIN_FAR_SCREENBLOCK,
	},
	[2] = {
		.priority = 1,
		.charblock = MY_CHARBLOCK,
		.screenblock = MOUNTAINS_SCREENBLOCK,
	},
	[3] = {
		.priority = 0,
		.charblock = MY_CHARBLOCK,
		.screenblock = TREES_SCREENBLOCK,
	}
};

void MainCB_parallaxMountainDusk(void (*_fadeCb)(void)) {
	fade_to_initialize(rgb(21, 13, 17));
	fadeCb = _fadeCb;
	scene_onframe_callback = &MainCB_parallaxMountainDusk_initFadeOut;
}

static void MainCB_parallaxMountainDusk_initFadeOut(void) {
	if (fade_step()) {
		scene_onframe_callback = &MainCB_parallaxMountainDusk_initFadeSolid;
	} else {
		fadeCb();
	}
}

static void MainCB_parallaxMountainDusk_initFadeSolid(void) {
	view_model = calloc(sizeof(view_model[0]), 1);

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_DISPCNT,
		.dispcnt = {
			.value = (dispcnt_t) {
				.mode = 0,
				.obj_character_mapping = OBJ_CHAR_MAP_1D,
				.force_blank = false,
				.enable_bg0 = true,
				.enable_bg1 = true,
				.enable_bg2 = true,
				.enable_bg3 = true,
				.enable_obj = false,
			}
		}
	});

	for (int i = 0; i < 4; i++) {
		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_HWREG_BGCNT,
			.bgcnt = {
				.value = my_bgcnts[i],
				.to_index = i,
			}
		});
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_COMPRESSED,
		.map_compressed = {
			.from = parallax_mountain_dusk_bg.tilemap,
			.to_block = SKY_SCREENBLOCK,
			.to_tile = 0,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {
				[0] = { .h = 8, .v = 0 },
				[1] = { .h = 0, .v = 0 },
				[2] = { .h = 0, .v = 0 },
				[3] = { .h = 0, .v = 0 },
			},
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES_COMPRESSED,
		.tiles_compressed = {
			.from = parallax_mountain_dusk_bg.tileset,
			.to_block = MY_CHARBLOCK,
			.to_tile = 0,
		},
	});

	unsigned transparent_tile_index = parallax_mountain_dusk_bg.tileset_count;

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES_FILL,
		.tiles_fill = {
			.value = 0,
			.to_block = MY_CHARBLOCK,
			.to_tile = transparent_tile_index,
			.count = 1,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = {transparent_tile_index},
			.to_block = MOUNTAIN_FAR_SCREENBLOCK,
			.to_tile = 0,
			.count = 20 * 32,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = {transparent_tile_index},
			.to_block = MOUNTAINS_SCREENBLOCK,
			.to_tile = 0,
			.count = (20 - parallax_mountain_dusk_mountains.tilemap_height) * 32,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = {transparent_tile_index},
			.to_block = TREES_SCREENBLOCK,
			.to_tile = 0,
			.count = (20 - parallax_mountain_dusk_trees.tilemap_height) * 32,
		},
	});

	for (unsigned x = 0; x < DISPLAY_WIDTH_TILES + 1; x++) {
		if (x >= MOUNTAINFAR_INITIAL_OFFSET) {
			vram_op_queue_enqueue((struct vram_op) {
				.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
				.map = {
					.from = parallax_mountain_dusk_mountain_far.tilemap + (x - MOUNTAINFAR_INITIAL_OFFSET) * parallax_mountain_dusk_mountain_far.tilemap_height,
					.to_block = MOUNTAIN_FAR_SCREENBLOCK,
					.to_tile = x + (20 - parallax_mountain_dusk_mountain_far.tilemap_height) * 32,
					.count = parallax_mountain_dusk_mountain_far.tilemap_height,
				},
			});
		}

		unsigned mountains_source_x = (x + parallax_mountain_dusk_mountains.tilemap_width - MOUNTAINS_INITIAL_OFFSET) % parallax_mountain_dusk_mountains.tilemap_width;
		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
			.map = {
				.from = parallax_mountain_dusk_mountains.tilemap + mountains_source_x * parallax_mountain_dusk_mountains.tilemap_height,
				.to_block = MOUNTAINS_SCREENBLOCK,
				.to_tile = x + (20 - parallax_mountain_dusk_mountains.tilemap_height) * 32,
				.count = parallax_mountain_dusk_mountains.tilemap_height,
			},
		});

		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
			.map = {
				.from = parallax_mountain_dusk_trees.tilemap + x * parallax_mountain_dusk_trees.tilemap_height,
				.to_block = TREES_SCREENBLOCK,
				.to_tile = x + (20 - parallax_mountain_dusk_trees.tilemap_height) * 32,
				.count = parallax_mountain_dusk_trees.tilemap_height,
			},
		});
	}

	view_model->mountain_far.next_hardware_column = DISPLAY_WIDTH_TILES + 1;
	view_model->mountain_far.next_source_column = DISPLAY_WIDTH_TILES + 1 - MOUNTAINFAR_INITIAL_OFFSET;

	view_model->mountains.next_hardware_column = DISPLAY_WIDTH_TILES + 1;
	view_model->mountains.next_source_column = DISPLAY_WIDTH_TILES + 1 - MOUNTAINS_INITIAL_OFFSET;

	view_model->trees.next_hardware_column = DISPLAY_WIDTH_TILES + 1;
	view_model->trees.next_source_column = DISPLAY_WIDTH_TILES + 1;

	fade_from_initialize(parallax_mountain_dusk_bg.palette[0], 16);
	scene_onframe_callback = &MainCB_parallaxMountainDusk_initFadeIn;
}

static void MainCB_parallaxMountainDusk_initFadeIn(void) {
	if (fade_step()) {
		scene_onframe_callback = &MainCB_parallaxMountainDusk_main;
	}
	MainCB_parallaxMountainDusk_main();
}

static void advance_layer(struct parallax_layer* layer, const struct background_horizontal_scroll* gfx, unsigned top_row, uint16_t screenblock, volatile uint16_t* hofsreg) {
	layer->hardware_offset++;
	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_UINT16,
		.uint16 = {
			.value = layer->hardware_offset,
			.to = hofsreg,
		},
	});

	if (layer->hardware_offset % 8 == 0) {
		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
			.map = {
				.from = gfx->tilemap +
					layer->next_source_column * gfx->tilemap_height,
				.to_block = screenblock,
				.to_tile = layer->next_hardware_column + top_row * 32,
				.count = gfx->tilemap_height,
			},
		});
		layer->next_source_column += 1;
		layer->next_source_column %= gfx->tilemap_width;

		layer->next_hardware_column += 1;
		layer->next_hardware_column %= 32;
	}
}

void MainCB_parallaxMountainDusk_main(void) {
	view_model->timer++;

	if (view_model->timer % 7 == 1) {
		advance_layer(
			&(view_model->mountain_far),
			&parallax_mountain_dusk_mountain_far,
			20 - parallax_mountain_dusk_mountain_far.tilemap_height,
			MOUNTAIN_FAR_SCREENBLOCK,
			&(reg_lcd.BGOFS[1].h)
		);
	}

	if (view_model->timer % 3 == 0) {
		advance_layer(
			&(view_model->mountains),
			&parallax_mountain_dusk_mountains,
			20 - parallax_mountain_dusk_mountains.tilemap_height,
			MOUNTAINS_SCREENBLOCK,
			&(reg_lcd.BGOFS[2].h)
		);
	}

	{
		advance_layer(
			&(view_model->trees),
			&parallax_mountain_dusk_trees,
			20 - parallax_mountain_dusk_trees.tilemap_height,
			TREES_SCREENBLOCK,
			&(reg_lcd.BGOFS[3].h)
		);
	}

	if (! keyinput_get_new().b) {
		scene_onframe_callback = &MainCB_parallaxMountainDusk_cleanup;
	}
}

static void MainCB_parallaxMountainDusk_cleanup(void) {
	if (view_model) {
		free(view_model);
		view_model = NULL;
	}

	scene_onframe_callback = &MainCB_mainMenu_init;
}
