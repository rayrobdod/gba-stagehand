#include "scene/parallax_mountain_dusk.h"

#include <stdlib.h>
#include "decompress/by_header.h"
#include "decompress/type.h"
#include "gba/screen.h"
#include "gba/vram.h"
#include "management/keyinput.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "utils/arraycount.h"
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

struct tree_sprite {
	bool enabled;
	uint8_t tile_num;
	int16_t offset;
};

[[gnu::section(".sbss.decompress_bg_tileset_buffer")]]
static tile_4bpp_t* decompress_bg_tileset_buffer = NULL;
[[gnu::section(".sbss.decompress_oam_tileset_buffer")]]
static tile_4bpp_t* decompress_oam_tileset_buffer = NULL;
[[gnu::section(".sbss.suspended_decompression")]]
static struct suspended_decompression* decompress_state = NULL;

[[gnu::section(".sbss.suspended_decompression_1")]]
static enum {
	SUSPENDED_DECOMP_NONE = 0,
	SUSPENDED_DECOMP_BG_TILES,
	SUSPENDED_DECOMP_OAM_TILES,
} decompress_state_state = SUSPENDED_DECOMP_NONE;

__attribute__((section(".sbss")))
static struct {
	uint16_t timer;
	struct parallax_layer mountain_far;
	struct parallax_layer mountains;
	struct parallax_layer trees;
	uint8_t next_tree_countdown;
	uint8_t next_tree_type;
	struct tree_sprite foreground_trees[8];
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

static inline size_t divceilmul(size_t numerator, size_t denominator) {
	return denominator * ((numerator / denominator) + (numerator % denominator ? 1 : 0));
}

void MainCB_parallaxMountainDusk(void (*_fadeCb)(void)) {
	fade_to_initialize(rgb(21, 13, 17));
	fadeCb = _fadeCb;
	scene_onframe_callback = &MainCB_parallaxMountainDusk_initFadeOut;
	decompress_bg_tileset_buffer = malloc(divceilmul(parallax_mountain_dusk_bg.tileset->size, sizeof(bg_tile_t)));
	decompress_oam_tileset_buffer = malloc(divceilmul(parallax_mountain_dusk_foreground_trees.tileset->size, sizeof(bg_tile_t)));
	decompress_state = calloc(sizeof(struct suspended_decompression), 1);
	HeaderUnCompSuspendableInit(decompress_state, parallax_mountain_dusk_bg.tileset, decompress_bg_tileset_buffer);
	decompress_state_state = SUSPENDED_DECOMP_BG_TILES;
}

static void MainCB_parallaxMountainDusk_initFadeOut(void) {
	if (fade_step() && decompress_state_state == SUSPENDED_DECOMP_NONE) {
		scene_onframe_callback = &MainCB_parallaxMountainDusk_initFadeSolid;
	} else {
		fadeCb();
		if (decompress_state_state != SUSPENDED_DECOMP_NONE) {
			if (HeaderUnCompSuspendable(decompress_state)) {
				switch (decompress_state_state) {
					case SUSPENDED_DECOMP_BG_TILES:
						HeaderUnCompSuspendableInit(decompress_state,
							parallax_mountain_dusk_foreground_trees.tileset, decompress_oam_tileset_buffer);
						decompress_state_state = SUSPENDED_DECOMP_OAM_TILES;
						break;
					case SUSPENDED_DECOMP_OAM_TILES:
						free(decompress_state);
						decompress_state = NULL;
						decompress_state_state = SUSPENDED_DECOMP_NONE;
						break;
					case SUSPENDED_DECOMP_NONE:
						break;
				}
			}
		}
	}
}

static void MainCB_parallaxMountainDusk_initFadeSolid(void) {
	view_model = calloc(sizeof(view_model[0]), 1);

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_DISPCNT,
		.dispcnt = {
			.value = (dispcnt_t) {0}
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

	bg_tile_t* map_bg = malloc(divceilmul(parallax_mountain_dusk_bg.tilemap->size, sizeof(bg_tile_t)));
	if (!map_bg) {
		MgbaPrintf(MGBA_LOG_ERROR, "Out of memory: parallaxMountainDusk map_bg");
		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_COMPRESSED,
			.map_compressed = {
				.from = parallax_mountain_dusk_bg.tilemap,
				.to_block = SKY_SCREENBLOCK,
				.to_tile = 0,
			},
		});
	} else {
		HeaderUnCompWram(parallax_mountain_dusk_bg.tilemap, map_bg);

		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = map_bg,
				.to_block = SKY_SCREENBLOCK,
				.to_tile = 0,
				.count = parallax_mountain_dusk_bg.tilemap_count,
			},
		});
       }

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
		.type = VRAM_QUEUE_OP_BG_TILES_FREE,
		.tiles_free = {
			.from = decompress_bg_tileset_buffer,
			.to_block = MY_CHARBLOCK,
			.to_tile = 0,
			.count = parallax_mountain_dusk_bg.tileset_count,
		},
	});
	decompress_bg_tileset_buffer = NULL;

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_OAM_TILES_FREE,
		.tiles_free = {
			.from = decompress_oam_tileset_buffer,
			.to_block = MY_CHARBLOCK,
			.to_tile = 0,
			.count = parallax_mountain_dusk_foreground_trees.tileset_count,
		},
	});
	decompress_oam_tileset_buffer = NULL;

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

	bg_tile_t* map_far = malloc(sizeof(bg_tile_t) * 20 * 32);
	bg_tile_t* map_mountains = malloc(sizeof(bg_tile_t) * 20 * 32);
	bg_tile_t* map_trees = malloc(sizeof(bg_tile_t) * 20 * 32);

	if (!map_far || !map_mountains || !map_trees) {
		MgbaPrintf(MGBA_LOG_ERROR, "Out of memory: parallaxMountainDusk map_far");
		exit(1);
	}

	for (unsigned i = 0; i < 20 * 32; i++) {
		map_far[i] = (bg_tile_t) {transparent_tile_index};
		map_mountains[i] = (bg_tile_t) {transparent_tile_index};
		map_trees[i] = (bg_tile_t) {transparent_tile_index};
	}

	for (unsigned x = MOUNTAINFAR_INITIAL_OFFSET; x < DISPLAY_WIDTH_TILES + 1; x++) {
		for (unsigned y = 0; y < parallax_mountain_dusk_mountain_far.tilemap_height; y++) {
			unsigned to_y = DISPLAY_HEIGHT_TILES - parallax_mountain_dusk_mountain_far.tilemap_height + y;

			map_far[x + 32 * to_y] = parallax_mountain_dusk_mountain_far.tilemap[
				(x - MOUNTAINFAR_INITIAL_OFFSET) * parallax_mountain_dusk_mountain_far.tilemap_height + y];
		}
	}

	for (unsigned x = 0; x < DISPLAY_WIDTH_TILES + 1; x++) {
		for (unsigned y = 0; y < parallax_mountain_dusk_mountains.tilemap_height; y++) {
			unsigned to_y = DISPLAY_HEIGHT_TILES - parallax_mountain_dusk_mountains.tilemap_height + y;

			map_mountains[x + 32 * to_y] = parallax_mountain_dusk_mountains.tilemap[
				(x - MOUNTAINS_INITIAL_OFFSET) * parallax_mountain_dusk_mountains.tilemap_height + y];
		}
	}

	for (unsigned x = 0; x < DISPLAY_WIDTH_TILES + 1; x++) {
		for (unsigned y = 0; y < parallax_mountain_dusk_trees.tilemap_height; y++) {
			unsigned to_y = 20 - parallax_mountain_dusk_trees.tilemap_height + y;

			map_trees[x + 32 * to_y] = parallax_mountain_dusk_trees.tilemap[
				x * parallax_mountain_dusk_trees.tilemap_height + y];
		}
	}

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FREE,
		.map_free = {
			.from = map_far,
			.to_block = MOUNTAIN_FAR_SCREENBLOCK,
			.to_tile = 0,
			.count = 20 * 32,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FREE,
		.map_free = {
			.from = map_mountains,
			.to_block = MOUNTAINS_SCREENBLOCK,
			.to_tile = 0,
			.count = 20 * 32,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FREE,
		.map_free = {
			.from = map_trees,
			.to_block = TREES_SCREENBLOCK,
			.to_tile = 0,
			.count = 20 * 32,
		},
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_DISPCNT,
		.dispcnt = {
			.value = (dispcnt_t) {
				.mode = 0,
				.obj_character_mapping = OBJ_CHAR_MAP_2D,
				.force_blank = false,
				.enable_bg0 = true,
				.enable_bg1 = true,
				.enable_bg2 = true,
				.enable_bg3 = true,
				.enable_obj = true,
			}
		}
	});

	view_model->mountain_far.next_hardware_column = DISPLAY_WIDTH_TILES + 1;
	view_model->mountain_far.next_source_column = (DISPLAY_WIDTH_TILES + 1 - MOUNTAINFAR_INITIAL_OFFSET) % parallax_mountain_dusk_mountain_far.tilemap_width;

	view_model->mountains.next_hardware_column = DISPLAY_WIDTH_TILES + 1;
	view_model->mountains.next_source_column = (DISPLAY_WIDTH_TILES + 1 - MOUNTAINS_INITIAL_OFFSET) % parallax_mountain_dusk_mountains.tilemap_width;

	view_model->trees.next_hardware_column = DISPLAY_WIDTH_TILES + 1;
	view_model->trees.next_source_column = DISPLAY_WIDTH_TILES + 1;

	view_model->next_tree_countdown = 16;

	union palette512 final_palette = {0};
	for (int i = 0; i < 16; i++)
		final_palette.background._4[0][i] = parallax_mountain_dusk_bg.palette[0][i];
	for (int i = 0; i < 16; i++)
		final_palette.object._4[0][i] = parallax_mountain_dusk_foreground_trees.palette[0][i];

	fade_from_initialize(final_palette.all, 512);
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


	static uint32_t lfsr = 0x67EDEEAE;

	if (0 == --(view_model->next_tree_countdown)) {
		for (unsigned i = 0; i < arraycount(view_model->foreground_trees); i++) {
			if (! view_model->foreground_trees[i].enabled) {
				view_model->foreground_trees[i] = (struct tree_sprite) {
					.enabled = true,
					.tile_num = view_model->next_tree_type * 4,
					.offset = DISPLAY_WIDTH,
				};
				break;
			}
		}
		view_model->next_tree_countdown = 8 + lfsr % 24;
		view_model->next_tree_type += 1;
		view_model->next_tree_type %= 7;
		lfsr = (lfsr >> 1) ^ (lfsr & 1 ? 0xB40000 : 0);
	}

	for (unsigned i = 0; i < arraycount(view_model->foreground_trees); i++) {
		if (view_model->foreground_trees[i].enabled) {
			view_model->foreground_trees[i].offset -= 2;
			if (view_model->foreground_trees[i].offset < -32) {
				view_model->foreground_trees[i].enabled = false;
			}
		}
		if (view_model->foreground_trees[i].enabled) {
			vram_op_queue_enqueue((struct vram_op) {
				.type = VRAM_QUEUE_OP_OAM_ENTRY,
				.oam = {
					.value = {
						.shape = OAM_SHAPE_VERTICAL,
						.size = 3,
						.y = DISPLAY_HEIGHT - 64,
						.x = view_model->foreground_trees[i].offset,
						.tile_num = view_model->foreground_trees[i].tile_num,
					},
					.to_index = i,
				},
			});
		}

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
