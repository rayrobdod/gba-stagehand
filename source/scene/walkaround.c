#include "scene/walkaround.h"

#include <stdlib.h>
#include <string.h>
#include "decompress/by_header.h"
#include "decompress/type.h"
#include "gba/screen.h"
#include "gba/vram.h"
#include "management/keyinput.h"
#include "management/shadow_oam.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "utils/arraycount.h"
#include "utils/minmax.h"
#include "utils/one_transparent_tileset.h"
#include "graphics.h"
#include "graphics_types.h"
#include "main.h"
#include "mgba.h"
#include "mix_rgb.h"

static void MainCB_walkaround_fadeout_black(void);
static void MainCB_walkaround_fadesolid_black(void);
static void MainCB_walkaround_fadein_black(void);
static void MainCB_walkaround_main(void);

static inline int floordiv16(int num) { return num >> 4; }
static inline int posmod16(int num) { return num & 15; }

enum direction {
	DIRECTION_NORTH,
	DIRECTION_SOUTH,
	DIRECTION_EAST,
	DIRECTION_WEST,
};

/* tile position relative to the top-left of the map */
typedef struct { int16_t x; int16_t y; } tile_coord_t;
/* pixel position relative to the top-left of the map */
typedef struct { int16_t x; int16_t y; } mapoffs_t;
/* pixel position relative to the screen */
typedef struct { int16_t x; int16_t y; } screenoffs_t;

__attribute__((section(".sbss")))
static void (*fadeCb)(void) = {0};

__attribute__((section(".sbss")))
static struct walkaround_viewmodel {
	struct {
		bgofs_t bgofs;
		mapoffs_t mapoffs;
	} camera;
	struct {
		shadow_oam_id_t oam_id;
		mapoffs_t mapoffs;
	} player;
} viewmodel = {0};

__attribute__((section(".sbss")))
static struct walkaround_model {
	const struct tile16x3map* map;
	struct {
		tile_coord_t pos;
	} player;
} state = {0};

static const struct tile16x3 tile16x3zero = {
	.behavior = WB_IMPASSABLE,
	.tiles = {{{0},{0},{0},{0}},{{0},{0},{0},{0}},{{0},{0},{0},{0}}},
};
static const struct walkaround_model initial_state = {
	.map = &mushroom_village,
	.player = {
		.pos = {.x = 7, .y = 7}
	},
};

enum {
	TERRAIN_CHARBLOCK = 0,
	HUD_CHARBLOCK = 2,

	HUD_SCREENBLOCK = 28,
	TERRAIN_SCREENBLOCK = 29,
};

static const struct shadow_vram_init walkaround_shadow_vram_init = {
	.enable_bg = {true, true, true, true},
	.enable_obj = true,
	.bgcnt = {
		[0] = {
			.priority = 0,
			.charblock = HUD_CHARBLOCK,
			.screenblock = HUD_SCREENBLOCK,
		},
		[1] = {
			.priority = 1,
			.charblock = TERRAIN_CHARBLOCK,
			.screenblock = TERRAIN_SCREENBLOCK,
		},
		[2] = {
			.priority = 2,
			.charblock = TERRAIN_CHARBLOCK,
			.screenblock = TERRAIN_SCREENBLOCK + 1,
		},
		[3] = {
			.priority = 3,
			.charblock = TERRAIN_CHARBLOCK,
			.screenblock = TERRAIN_SCREENBLOCK + 2,
		}
	}
};


static const struct tile16x3* metatile_at(const struct tile16x3map* map, signed x, signed y) {
	if (x < 0 || y < 0 || x >= map->width || y >= map->height) {
		return &tile16x3zero;
	} else {
		return &map->metatileset[map->metatilemap[x + map->width * y]];
	}
}

static mapoffs_t mapoffs_add(mapoffs_t a, mapoffs_t b) {
	mapoffs_t retval = {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
	return retval;
};
static mapoffs_t tile_coord_to_pixel_coord(tile_coord_t in) {
	mapoffs_t retval = {
		.x = 16 * in.x,
		.y = 16 * in.y,
	};
	return retval;
}
static screenoffs_t pixel_coord_to_screen_coord(mapoffs_t in) {
	screenoffs_t retval = {
		.x = in.x - viewmodel.camera.mapoffs.x,
		.y = in.y - viewmodel.camera.mapoffs.y,
	};
	return retval;
}
static mapoffs_t player_tile_coord_to_target_pixel_coord(tile_coord_t in) {
	return mapoffs_add(tile_coord_to_pixel_coord(in), (mapoffs_t) {8, 14});
}

void ChangeScene_walkaround(void (*_fadeCb)(void)) {
	fade_to_initialize(rgb(0, 0, 0));
	fadeCb = _fadeCb;
	scene_onframe_callback = &MainCB_walkaround_fadeout_black;
}

static void MainCB_walkaround_fadeout_black(void) {
	if (fade_step()) {
		scene_onframe_callback = &MainCB_walkaround_fadesolid_black;
	} else {
		fadeCb();
	}
}

static union palette512 MainCB_walkaround_fadesolid(void) {
	shadow_vram_init(&walkaround_shadow_vram_init);
	shadow_oam_init();

	state = initial_state;
	viewmodel = (struct walkaround_viewmodel) {0};

	shadow_tiles_load_tileset(&state.map->tileset, (struct shadow_tiles_load_tileset) {.bg = 1});
	shadow_tiles_load_tileset(&one_transparent_tileset, (struct shadow_tiles_load_tileset) {.bg = 0});

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = {0},
			.to_block = HUD_SCREENBLOCK,
			.to_tile = 0,
			.count = sizeof(screenblock_t) / sizeof(bg_tile_t),
		},
	});

	tile_coord_t tile_mapoffs = {
		.x = state.player.pos.x - 7,
		.y = state.player.pos.y - 5,
	};
	viewmodel.camera.mapoffs = tile_coord_to_pixel_coord(tile_mapoffs);

	bg_tile_t* terrain = malloc(3 * sizeof(screenblock_t));
	if (terrain) {
		memset(terrain, 0, 3 * sizeof(screenblock_t));

		for (signed screen_y = 0; screen_y < 10; screen_y++)
		for (signed screen_x = 0; screen_x < 15; screen_x++)
		{
			signed model_y = screen_y + tile_mapoffs.y;
			signed model_x = screen_x + tile_mapoffs.x;

			const struct tile16x3* metatile = metatile_at(state.map, model_x, model_y);
			unsigned position_in_layer = screen_y * 32 * 2 + screen_x * 2;

			for (unsigned layer = 0; layer < 3; ++layer) {
				unsigned position_of_layer = layer * sizeof(screenblock_t) / sizeof(bg_tile_t);
				unsigned position = position_of_layer + position_in_layer;

				terrain[position] = metatile->tiles[layer][0];
				terrain[position + 1] = metatile->tiles[layer][1];
				terrain[position + 32] = metatile->tiles[layer][2];
				terrain[position + 33] = metatile->tiles[layer][3];
			}
		}

		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = terrain,
				.to_block = TERRAIN_SCREENBLOCK,
				.to_tile = 0,
				.count = 3 * sizeof(screenblock_t) / sizeof(bg_tile_t),
			},
		});
	}

	viewmodel.player.mapoffs = player_tile_coord_to_target_pixel_coord(state.player.pos);
	screenoffs_t player_screenpos = pixel_coord_to_screen_coord(viewmodel.player.mapoffs);
	struct shadow_oam_add_sprite_no_palette_vram_op player_oam =
		shadow_oam_add_sprite_no_palette_vram_op(
			&character_base_male_west_idle1,
			(struct shadow_oam_position) {
				.coord = {.x = player_screenpos.x, .y = player_screenpos.y},
				.hotspot = HOTSPOT_BOTTOM,
				.hflip = false,
				.vflip = false,
				.priority = 2,
			});
	viewmodel.player.oam_id = player_oam.sprite_index;

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {{0},{0},{0},{0},},
		},
	});

	union palette512 final_palette = {0};
	memcpy(final_palette.background._4[0], state.map->tileset.palette, 16 * 2 * 12);
	memcpy(final_palette.object._4[player_oam.palette_index], character_base_male_west_idle1.palette, 16);
	return final_palette;
}

static void MainCB_walkaround_fadesolid_black(void) {
	union palette512 final_palette = MainCB_walkaround_fadesolid();
	fade_from_initialize(final_palette.all, 512);
	scene_onframe_callback = &MainCB_walkaround_fadein_black;
}

static void MainCB_walkaround_fadein_black(void) {
	if (fade_step()) {
		scene_onframe_callback = &MainCB_walkaround_main;
	}
	MainCB_walkaround_main();
}

static screenoffs_t center_player_in_camera_target(void) {
	screenoffs_t retval = {
		.x = viewmodel.player.mapoffs.x - 7 * 16 - 8,
		.y = viewmodel.player.mapoffs.y - 5 * 16 - 14,
	};
	return retval;
}

/*
 * @pre speed is less than 8
 * @return whether the camera was moved
 */
static bool move_camera_towards(screenoffs_t target_mapoffs, int speed) {
	bool retval = false;

	if (target_mapoffs.x < viewmodel.camera.mapoffs.x) {
		retval = true;
		int delta = min(speed, viewmodel.camera.mapoffs.x - target_mapoffs.x);
		viewmodel.camera.mapoffs.x -= delta;
		int bgofs_after = viewmodel.camera.bgofs.h - delta;
		if ((viewmodel.camera.bgofs.h & ~0x0F) != (bgofs_after & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_x = posmod16(floordiv16(bgofs_after)) * 2;
				signed model_x = floordiv16(viewmodel.camera.mapoffs.x);

				signed tile_mapoffs_y = floordiv16(viewmodel.camera.mapoffs.y);
				signed tile_bgofs_y = floordiv16(viewmodel.camera.bgofs.v);

				for (signed dy = 0; dy < 16; dy++) {
					signed screen_y = posmod16(dy + tile_bgofs_y);
					signed model_y = dy + tile_mapoffs_y;

					const struct tile16x3* metatile = metatile_at(state.map, model_x, model_y);
					unsigned position_in_layer = screen_y * 2;

					for (unsigned layer = 0; layer < 3; ++layer) {
						unsigned position_of_layer = layer * 32 * 2;
						unsigned position = position_of_layer + position_in_layer;

						terrain[position] = metatile->tiles[layer][0];
						terrain[position + 1] = metatile->tiles[layer][2];
						terrain[position + 32] = metatile->tiles[layer][1];
						terrain[position + 33] = metatile->tiles[layer][3];
					}
				}

				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 5,
						.to_block = TERRAIN_SCREENBLOCK + 2,
						.to_tile = screen_x + 1,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 4,
						.to_block = TERRAIN_SCREENBLOCK + 2,
						.to_tile = screen_x,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 3,
						.to_block = TERRAIN_SCREENBLOCK + 1,
						.to_tile = screen_x + 1,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 2,
						.to_block = TERRAIN_SCREENBLOCK + 1,
						.to_tile = screen_x,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 1,
						.to_block = TERRAIN_SCREENBLOCK,
						.to_tile = screen_x + 1,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN_FREE,
					.map_free = {
						.from = terrain,
						.to_block = TERRAIN_SCREENBLOCK,
						.to_tile = screen_x,
						.count = 32,
					},
				});
			}
		}
		viewmodel.camera.bgofs.h = bgofs_after;
	}
	if (target_mapoffs.x > viewmodel.camera.mapoffs.x) {
		retval = true;
		int delta = min(speed, target_mapoffs.x - viewmodel.camera.mapoffs.x);
		viewmodel.camera.mapoffs.x += delta;
		int bgofs_after = viewmodel.camera.bgofs.h + delta;
		if (((viewmodel.camera.bgofs.h - 1) & ~0x0F) != ((bgofs_after - 1) & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_x = posmod16(floordiv16(bgofs_after) + 15) * 2;
				signed model_x = floordiv16(viewmodel.camera.mapoffs.x) + 15;

				signed tile_mapoffs_y = floordiv16(viewmodel.camera.mapoffs.y);
				signed tile_bgofs_y = floordiv16(viewmodel.camera.bgofs.v);

				for (signed dx = 0; dx < 16; dx++) {
					signed screen_y = posmod16(dx + tile_bgofs_y);
					signed model_y = dx + tile_mapoffs_y;

					const struct tile16x3* metatile = metatile_at(state.map, model_x, model_y);
					unsigned position_in_layer = screen_y * 2;

					for (unsigned layer = 0; layer < 3; ++layer) {
						unsigned position_of_layer = layer * 32 * 2;
						unsigned position = position_of_layer + position_in_layer;

						terrain[position] = metatile->tiles[layer][0];
						terrain[position + 1] = metatile->tiles[layer][2];
						terrain[position + 32] = metatile->tiles[layer][1];
						terrain[position + 33] = metatile->tiles[layer][3];
					}
				}

				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 5,
						.to_block = TERRAIN_SCREENBLOCK + 2,
						.to_tile = screen_x + 1,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 4,
						.to_block = TERRAIN_SCREENBLOCK + 2,
						.to_tile = screen_x,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 3,
						.to_block = TERRAIN_SCREENBLOCK + 1,
						.to_tile = screen_x + 1,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 2,
						.to_block = TERRAIN_SCREENBLOCK + 1,
						.to_tile = screen_x,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN,
					.map = {
						.from = terrain + 32 * 1,
						.to_block = TERRAIN_SCREENBLOCK,
						.to_tile = screen_x + 1,
						.count = 32,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_COLUMN_FREE,
					.map_free = {
						.from = terrain,
						.to_block = TERRAIN_SCREENBLOCK,
						.to_tile = screen_x,
						.count = 32,
					},
				});
			}
		}
		viewmodel.camera.bgofs.h = bgofs_after;
	}
	if (target_mapoffs.y < viewmodel.camera.mapoffs.y) {
		retval = true;
		int delta = min(speed, viewmodel.camera.mapoffs.y - target_mapoffs.y);
		viewmodel.camera.mapoffs.y -= delta;
		int bgofs_after = viewmodel.camera.bgofs.v - delta;
		if ((viewmodel.camera.bgofs.v & ~0x0F) != (bgofs_after & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_y = posmod16(floordiv16(bgofs_after)) * 2;
				signed model_y = floordiv16(viewmodel.camera.mapoffs.y);

				signed tile_mapoffs_x = floordiv16(viewmodel.camera.mapoffs.x);
				signed tile_bgofs_x = floordiv16(viewmodel.camera.bgofs.h);

				for (signed dx = 0; dx < 16; dx++) {
					signed screen_x = posmod16(dx + tile_bgofs_x);
					signed model_x = dx + tile_mapoffs_x;

					const struct tile16x3* metatile = metatile_at(state.map, model_x, model_y);
					unsigned position_in_layer = screen_x * 2;

					for (unsigned layer = 0; layer < 3; ++layer) {
						unsigned position_of_layer = layer * 32 * 2;
						unsigned position = position_of_layer + position_in_layer;

						terrain[position] = metatile->tiles[layer][0];
						terrain[position + 1] = metatile->tiles[layer][1];
						terrain[position + 32] = metatile->tiles[layer][2];
						terrain[position + 33] = metatile->tiles[layer][3];
					}
				}

				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP,
					.map = {
						.from = terrain + 32 * 2 * 2,
						.to_block = TERRAIN_SCREENBLOCK + 2,
						.to_tile = 32 * screen_y,
						.count = 32 * 2,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP,
					.map = {
						.from = terrain + 32 * 2,
						.to_block = TERRAIN_SCREENBLOCK + 1,
						.to_tile = 32 * screen_y,
						.count = 32 * 2,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_FREE,
					.map_free = {
						.from = terrain,
						.to_block = TERRAIN_SCREENBLOCK,
						.to_tile = 32 * screen_y,
						.count = 32 * 2,
					},
				});
			}
		}
		viewmodel.camera.bgofs.v = bgofs_after;
	}
	if (target_mapoffs.y > viewmodel.camera.mapoffs.y) {
		retval = true;
		int delta = min(speed, target_mapoffs.y - viewmodel.camera.mapoffs.y);
		viewmodel.camera.mapoffs.y += delta;
		int bgofs_after = viewmodel.camera.bgofs.v + delta;
		if (((viewmodel.camera.bgofs.v - 1) & ~0x0F) != ((bgofs_after - 1) & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_y = posmod16(floordiv16(bgofs_after) + 10) * 2;
				signed model_y = floordiv16(viewmodel.camera.mapoffs.y) + 10;

				signed tile_mapoffs_x = floordiv16(viewmodel.camera.mapoffs.x);
				signed tile_bgofs_x = floordiv16(viewmodel.camera.bgofs.h);

				for (signed dx = 0; dx < 16; dx++) {
					signed screen_x = posmod16(dx + tile_bgofs_x);
					signed model_x = dx + tile_mapoffs_x;

					const struct tile16x3* metatile = metatile_at(state.map, model_x, model_y);
					unsigned position_in_layer = screen_x * 2;

					for (unsigned layer = 0; layer < 3; ++layer) {
						unsigned position_of_layer = layer * 32 * 2;
						unsigned position = position_of_layer + position_in_layer;

						terrain[position] = metatile->tiles[layer][0];
						terrain[position + 1] = metatile->tiles[layer][1];
						terrain[position + 32] = metatile->tiles[layer][2];
						terrain[position + 33] = metatile->tiles[layer][3];
					}
				}

				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP,
					.map = {
						.from = terrain + 32 * 2 * 2,
						.to_block = TERRAIN_SCREENBLOCK + 2,
						.to_tile = 32 * screen_y,
						.count = 32 * 2,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP,
					.map = {
						.from = terrain + 32 * 2,
						.to_block = TERRAIN_SCREENBLOCK + 1,
						.to_tile = 32 * screen_y,
						.count = 32 * 2,
					},
				});
				vram_op_queue_enqueue((struct vram_op) {
					.type = VRAM_QUEUE_OP_BG_MAP_FREE,
					.map_free = {
						.from = terrain,
						.to_block = TERRAIN_SCREENBLOCK,
						.to_tile = 32 * screen_y,
						.count = 32 * 2,
					},
				});
			}
		}
		viewmodel.camera.bgofs.v = bgofs_after;
	}

	if (retval) {
		struct bgofs bgofs = {
			.h = viewmodel.camera.bgofs.h,
			.v = viewmodel.camera.bgofs.v,
		};
		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
			.bgofss = {
				.value = {{0},bgofs,bgofs,bgofs,},
			},
		});
	}

	return retval;
}

struct direction_info {
	int8_t dx;
	int8_t dy;
	bool (*can_leave)(enum WalkaroundBehavior);
	bool (*can_enter)(enum WalkaroundBehavior);
};

static bool can_leave_north(enum WalkaroundBehavior wb) {
	return wb != WB_IMPASSABLE &&
		wb != WB_IMPASSABLE_N &&
		wb != WB_IMPASSABLE_NE &&
		wb != WB_IMPASSABLE_NW &&
		wb != WB_IMPASSABLE_NS &&
		true;
}
static bool can_leave_south(enum WalkaroundBehavior wb) {
	return wb != WB_IMPASSABLE &&
		wb != WB_IMPASSABLE_S &&
		wb != WB_IMPASSABLE_SE &&
		wb != WB_IMPASSABLE_SW &&
		wb != WB_IMPASSABLE_NS &&
		true;
}
static bool can_leave_east(enum WalkaroundBehavior wb) {
	return wb != WB_IMPASSABLE &&
		wb != WB_IMPASSABLE_E &&
		wb != WB_IMPASSABLE_NE &&
		wb != WB_IMPASSABLE_SE &&
		wb != WB_IMPASSABLE_EW &&
		true;
}
static bool can_leave_west(enum WalkaroundBehavior wb) {
	return wb != WB_IMPASSABLE &&
		wb != WB_IMPASSABLE_W &&
		wb != WB_IMPASSABLE_NW &&
		wb != WB_IMPASSABLE_SW &&
		wb != WB_IMPASSABLE_EW &&
		true;
}

static const struct direction_info direction_infos[] = {
	[DIRECTION_NORTH] = {
		.dx = 0,
		.dy = -1,
		.can_leave = &can_leave_north,
		.can_enter = &can_leave_south,
	},
	[DIRECTION_SOUTH] = {
		.dx = 0,
		.dy = 1,
		.can_leave = &can_leave_south,
		.can_enter = &can_leave_north,
	},
	[DIRECTION_EAST] = {
		.dx = 1,
		.dy = 0,
		.can_leave = &can_leave_east,
		.can_enter = &can_leave_west,
	},
	[DIRECTION_WEST] = {
		.dx = -1,
		.dy = 0,
		.can_leave = &can_leave_west,
		.can_enter = &can_leave_east,
	},
};

static bool can_move_in_direction(tile_coord_t from, enum direction direction) {
	const struct direction_info* direction_info = &direction_infos[direction];

	enum WalkaroundBehavior current_behavior = metatile_at(state.map, from.x, from.y)->behavior;
	enum WalkaroundBehavior target_behavior = metatile_at(state.map, from.x + direction_info->dx, from.y + direction_info->dy)->behavior;

	return direction_info->can_leave(current_behavior) && direction_info->can_enter(target_behavior);
}

static const int PLAYER_SPEED = 2;

/*
 * @return whether the player sprite was moved or changed
 */
static bool normal_player_movement(void) {
	mapoffs_t target_mapoffs = player_tile_coord_to_target_pixel_coord(state.player.pos);
	if (viewmodel.player.mapoffs.x != target_mapoffs.x || viewmodel.player.mapoffs.y != target_mapoffs.y) {
		if (viewmodel.player.mapoffs.x < target_mapoffs.x) {
			viewmodel.player.mapoffs.x += PLAYER_SPEED;
		}
		else if (viewmodel.player.mapoffs.x > target_mapoffs.x) {
			viewmodel.player.mapoffs.x -= PLAYER_SPEED;
		}
		else if (viewmodel.player.mapoffs.y < target_mapoffs.y) {
			viewmodel.player.mapoffs.y += PLAYER_SPEED;
		}
		else if (viewmodel.player.mapoffs.y > target_mapoffs.y) {
			viewmodel.player.mapoffs.y -= PLAYER_SPEED;
		}

		return true;
	} else {
		keypad_t inputs = keyinput_get_down();

		// if holding down both a horizontal direction and a vertical direction,
		// the `(state.player.pos.x % 2 == state.player.pos.y % 2)` part will make the player alternate between moving horizontally and vertically
		if (! inputs.right && (state.player.pos.x % 2 == state.player.pos.y % 2) && can_move_in_direction(state.player.pos, DIRECTION_EAST)) {
			state.player.pos.x += 1;
			viewmodel.player.mapoffs.x += PLAYER_SPEED;
			return true;
		}
		else if (! inputs.left && (state.player.pos.x % 2 == state.player.pos.y % 2) && can_move_in_direction(state.player.pos, DIRECTION_WEST)) {
			state.player.pos.x -= 1;
			viewmodel.player.mapoffs.x -= PLAYER_SPEED;
			return true;
		}
		else if (! inputs.up && can_move_in_direction(state.player.pos, DIRECTION_NORTH)) {
			state.player.pos.y -= 1;
			viewmodel.player.mapoffs.y -= PLAYER_SPEED;
			return true;
		}
		else if (! inputs.down && can_move_in_direction(state.player.pos, DIRECTION_SOUTH)) {
			state.player.pos.y += 1;
			viewmodel.player.mapoffs.y += PLAYER_SPEED;
			return true;
		}
		else if (! inputs.right && can_move_in_direction(state.player.pos, DIRECTION_EAST)) {
			state.player.pos.x += 1;
			viewmodel.player.mapoffs.x += PLAYER_SPEED;
			return true;
		}
		else if (! inputs.left && can_move_in_direction(state.player.pos, DIRECTION_WEST)) {
			state.player.pos.x -= 1;
			viewmodel.player.mapoffs.x -= PLAYER_SPEED;
			return true;
		}
	}

	return false;
}

static void MainCB_walkaround_main(void) {
	bool refresh_player = false;

	refresh_player |= normal_player_movement();

	refresh_player |= move_camera_towards(center_player_in_camera_target(), 2);

	if (refresh_player) {
		screenoffs_t player_screenpos = pixel_coord_to_screen_coord(viewmodel.player.mapoffs);
		shadow_oam_move_sprite(
			viewmodel.player.oam_id,
			(struct shadow_oam_position) {
				.coord = {.x = player_screenpos.x, .y = player_screenpos.y},
				.hotspot = HOTSPOT_BOTTOM,
				.hflip = false,
				.vflip = false,
				.priority = 2,
			});
	}
}
