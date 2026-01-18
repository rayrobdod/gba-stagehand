#include "scene/walkaround.h"
#include "scene/walkaround_intern.h"

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
#include "main.h"
#include "mgba.h"
#include "mix_rgb.h"

static void MainCB_walkaround_fadeout_black(void);
static void MainCB_walkaround_fadesolid_black(void);
static void MainCB_walkaround_fadein_black(void);

static const int PLAYER_MOVE_SPEED = 1;
static const int TURN_FRAMES = 4;
static const uint8_t IDLE_ANIM_SPEED = 12;
static const uint8_t WALK_ANIM_SPEED = 4;

static const uint8_t OAM_INDEX_GOTO = 0xFF;

static inline int floordiv16(int num) { return num >> 4; }
static inline int posmod16(int num) { return num & 15; }

struct oam_animation_cell {
	uint8_t oam_index;
	uint8_t delay;
};

struct oam_animation {
	const struct shadow_oam_template * const * oams;
	const struct oam_animation_cell* frames;
};

#define FOREACH_DIRECTION(F) \
	F(north) \
	F(south) \
	F(east) \
	F(west)

static const struct oam_animation_cell idle_frames[] = {
	{0, 2*IDLE_ANIM_SPEED},
	{1, 1*IDLE_ANIM_SPEED},
	{2, 1*IDLE_ANIM_SPEED},
	{1, 2*IDLE_ANIM_SPEED},
	{0, 2*IDLE_ANIM_SPEED},
	{OAM_INDEX_GOTO, 0},
};

static const struct oam_animation_cell walking_frames[] = {
	{0, WALK_ANIM_SPEED},
	{1, WALK_ANIM_SPEED},
	{2, WALK_ANIM_SPEED},
	{3, WALK_ANIM_SPEED},
	{4, WALK_ANIM_SPEED},
	{5, WALK_ANIM_SPEED},
	{6, WALK_ANIM_SPEED},
	{7, WALK_ANIM_SPEED},
	{OAM_INDEX_GOTO, 0}
};


#define BASE_MALE_IDLE_OAMS(dir) \
	static const struct shadow_oam_template* const character_base_male_##dir##_idle_oams[] = { \
		&character_base_male_##dir##_idle1, \
		&character_base_male_##dir##_idle2, \
		&character_base_male_##dir##_idle3, \
	};
FOREACH_DIRECTION(BASE_MALE_IDLE_OAMS)

#define BASE_MALE_IDLE(dir) \
	static const struct oam_animation character_base_male_##dir##_idle = { \
		character_base_male_##dir##_idle_oams, \
		idle_frames, \
	};
FOREACH_DIRECTION(BASE_MALE_IDLE)

#define BASE_MALE_WALKING_OAMS(dir) \
	static const struct shadow_oam_template* const character_base_male_##dir##_walking_oams[] = { \
		&character_base_male_##dir##_walking1, \
		&character_base_male_##dir##_walking2, \
		&character_base_male_##dir##_walking3, \
		&character_base_male_##dir##_walking4, \
		&character_base_male_##dir##_walking5, \
		&character_base_male_##dir##_walking6, \
		&character_base_male_##dir##_walking7, \
		&character_base_male_##dir##_walking8, \
	};
FOREACH_DIRECTION(BASE_MALE_WALKING_OAMS)

#define BASE_MALE_WALKING(dir) \
	static const struct oam_animation character_base_male_##dir##_walking = { \
		character_base_male_##dir##_walking_oams, \
		walking_frames, \
	};
FOREACH_DIRECTION(BASE_MALE_WALKING)



__attribute__((section(".sbss")))
static void (*fadeCb)(void) = {0};

__attribute__((section(".sbss")))
struct walkaround_viewmodel walkaround_viewmodel = {0};

__attribute__((section(".sbss")))
struct walkaround_model walkaround_state = {0};

static const struct tile16x3 tile16x3zero = {
	.behavior = WB_IMPASSABLE,
	.tiles = {{{0},{0},{0},{0}},{{0},{0},{0},{0}},{{0},{0},{0},{0}}},
};
static const struct walkaround_model initial_state = {
	.map = &mushroom_village,
	.player = {
		.pos = {.x = 7, .y = 7},
		.turn_timer = 0,
		.action = ACTION_NONE,
		.facing = DIRECTION_WEST,
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
		.x = in.x - walkaround_viewmodel.camera.mapoffs.x,
		.y = in.y - walkaround_viewmodel.camera.mapoffs.y,
	};
	return retval;
}
static mapoffs_t player_tile_coord_to_target_pixel_coord(tile_coord_t in) {
	return mapoffs_add(tile_coord_to_pixel_coord(in), (mapoffs_t) {8, 14});
}

static struct shadow_oam_position player_oam_position(mapoffs_t player_mappos) {
	screenoffs_t player_screenpos = pixel_coord_to_screen_coord(player_mappos);
	return (struct shadow_oam_position) {
		.coord = {.x = player_screenpos.x, .y = player_screenpos.y},
		.hotspot = HOTSPOT_BOTTOM,
		.hflip = false,
		.vflip = false,
		.priority = 2,
	};
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

	walkaround_state = initial_state;
	walkaround_viewmodel = (struct walkaround_viewmodel) {0};

	shadow_tiles_load_tileset(&walkaround_state.map->tileset, (struct shadow_tiles_load_tileset) {.bg = 1});
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
		.x = walkaround_state.player.pos.x - 7,
		.y = walkaround_state.player.pos.y - 5,
	};
	walkaround_viewmodel.camera.mapoffs = tile_coord_to_pixel_coord(tile_mapoffs);

	bg_tile_t* terrain = malloc(3 * sizeof(screenblock_t));
	if (terrain) {
		memset(terrain, 0, 3 * sizeof(screenblock_t));

		for (signed screen_y = 0; screen_y < 10; screen_y++)
		for (signed screen_x = 0; screen_x < 15; screen_x++)
		{
			signed model_y = screen_y + tile_mapoffs.y;
			signed model_x = screen_x + tile_mapoffs.x;

			const struct tile16x3* metatile = metatile_at(walkaround_state.map, model_x, model_y);
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

	walkaround_viewmodel.player.mapoffs = player_tile_coord_to_target_pixel_coord(walkaround_state.player.pos);
	walkaround_viewmodel.player.anim = &character_base_male_west_idle;
	walkaround_viewmodel.player.anim_frame = 0;
	walkaround_viewmodel.player.anim_delay = walkaround_viewmodel.player.anim->frames[0].delay;
	struct shadow_oam_add_sprite_no_palette_vram_op player_oam =
		shadow_oam_add_sprite_no_palette_vram_op(
			walkaround_viewmodel.player.anim->oams[
				walkaround_viewmodel.player.anim->frames[0].oam_index],
			player_oam_position(walkaround_viewmodel.player.mapoffs));
	walkaround_viewmodel.player.oam_id = player_oam.sprite_index;


	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {{0},{0},{0},{0},},
		},
	});

	union palette512 final_palette = {0};
	memcpy(final_palette.background._4[0], walkaround_state.map->tileset.palette, 16 * 2 * 12);
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
		.x = walkaround_viewmodel.player.mapoffs.x - 7 * 16 - 8,
		.y = walkaround_viewmodel.player.mapoffs.y - 5 * 16 - 14,
	};
	return retval;
}

/*
 * @pre speed is less than 8
 * @return whether the camera was moved
 */
static bool move_camera_towards(screenoffs_t target_mapoffs, int speed) {
	bool retval = false;

	if (target_mapoffs.x < walkaround_viewmodel.camera.mapoffs.x) {
		retval = true;
		int delta = min(speed, walkaround_viewmodel.camera.mapoffs.x - target_mapoffs.x);
		walkaround_viewmodel.camera.mapoffs.x -= delta;
		int bgofs_after = walkaround_viewmodel.camera.bgofs.h - delta;
		if ((walkaround_viewmodel.camera.bgofs.h & ~0x0F) != (bgofs_after & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_x = posmod16(floordiv16(bgofs_after)) * 2;
				signed model_x = floordiv16(walkaround_viewmodel.camera.mapoffs.x);

				signed tile_mapoffs_y = floordiv16(walkaround_viewmodel.camera.mapoffs.y);
				signed tile_bgofs_y = floordiv16(walkaround_viewmodel.camera.bgofs.v);

				for (signed dy = 0; dy < 16; dy++) {
					signed screen_y = posmod16(dy + tile_bgofs_y);
					signed model_y = dy + tile_mapoffs_y;

					const struct tile16x3* metatile = metatile_at(walkaround_state.map, model_x, model_y);
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
		walkaround_viewmodel.camera.bgofs.h = bgofs_after;
	}
	if (target_mapoffs.x > walkaround_viewmodel.camera.mapoffs.x) {
		retval = true;
		int delta = min(speed, target_mapoffs.x - walkaround_viewmodel.camera.mapoffs.x);
		walkaround_viewmodel.camera.mapoffs.x += delta;
		int bgofs_after = walkaround_viewmodel.camera.bgofs.h + delta;
		if (((walkaround_viewmodel.camera.bgofs.h - 1) & ~0x0F) != ((bgofs_after - 1) & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_x = posmod16(floordiv16(bgofs_after) + 15) * 2;
				signed model_x = floordiv16(walkaround_viewmodel.camera.mapoffs.x) + 15;

				signed tile_mapoffs_y = floordiv16(walkaround_viewmodel.camera.mapoffs.y);
				signed tile_bgofs_y = floordiv16(walkaround_viewmodel.camera.bgofs.v);

				for (signed dx = 0; dx < 16; dx++) {
					signed screen_y = posmod16(dx + tile_bgofs_y);
					signed model_y = dx + tile_mapoffs_y;

					const struct tile16x3* metatile = metatile_at(walkaround_state.map, model_x, model_y);
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
		walkaround_viewmodel.camera.bgofs.h = bgofs_after;
	}
	if (target_mapoffs.y < walkaround_viewmodel.camera.mapoffs.y) {
		retval = true;
		int delta = min(speed, walkaround_viewmodel.camera.mapoffs.y - target_mapoffs.y);
		walkaround_viewmodel.camera.mapoffs.y -= delta;
		int bgofs_after = walkaround_viewmodel.camera.bgofs.v - delta;
		if ((walkaround_viewmodel.camera.bgofs.v & ~0x0F) != (bgofs_after & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_y = posmod16(floordiv16(bgofs_after)) * 2;
				signed model_y = floordiv16(walkaround_viewmodel.camera.mapoffs.y);

				signed tile_mapoffs_x = floordiv16(walkaround_viewmodel.camera.mapoffs.x);
				signed tile_bgofs_x = floordiv16(walkaround_viewmodel.camera.bgofs.h);

				for (signed dx = 0; dx < 16; dx++) {
					signed screen_x = posmod16(dx + tile_bgofs_x);
					signed model_x = dx + tile_mapoffs_x;

					const struct tile16x3* metatile = metatile_at(walkaround_state.map, model_x, model_y);
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
		walkaround_viewmodel.camera.bgofs.v = bgofs_after;
	}
	if (target_mapoffs.y > walkaround_viewmodel.camera.mapoffs.y) {
		retval = true;
		int delta = min(speed, target_mapoffs.y - walkaround_viewmodel.camera.mapoffs.y);
		walkaround_viewmodel.camera.mapoffs.y += delta;
		int bgofs_after = walkaround_viewmodel.camera.bgofs.v + delta;
		if (((walkaround_viewmodel.camera.bgofs.v - 1) & ~0x0F) != ((bgofs_after - 1) & ~0x0F)) {
			bg_tile_t* terrain = malloc(3 * 32 * 2 * sizeof(bg_tile_t));
			if (terrain) {
				signed screen_y = posmod16(floordiv16(bgofs_after) + 10) * 2;
				signed model_y = floordiv16(walkaround_viewmodel.camera.mapoffs.y) + 10;

				signed tile_mapoffs_x = floordiv16(walkaround_viewmodel.camera.mapoffs.x);
				signed tile_bgofs_x = floordiv16(walkaround_viewmodel.camera.bgofs.h);

				for (signed dx = 0; dx < 16; dx++) {
					signed screen_x = posmod16(dx + tile_bgofs_x);
					signed model_x = dx + tile_mapoffs_x;

					const struct tile16x3* metatile = metatile_at(walkaround_state.map, model_x, model_y);
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
		walkaround_viewmodel.camera.bgofs.v = bgofs_after;
	}

	if (retval) {
		struct bgofs bgofs = {
			.h = walkaround_viewmodel.camera.bgofs.h,
			.v = walkaround_viewmodel.camera.bgofs.v,
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
	const struct oam_animation* idle_anim;
	const struct oam_animation* walking_anim;
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
		.idle_anim = &character_base_male_north_idle,
		.walking_anim = &character_base_male_north_walking,
	},
	[DIRECTION_SOUTH] = {
		.dx = 0,
		.dy = 1,
		.can_leave = &can_leave_south,
		.can_enter = &can_leave_north,
		.idle_anim = &character_base_male_south_idle,
		.walking_anim = &character_base_male_south_walking,
	},
	[DIRECTION_EAST] = {
		.dx = 1,
		.dy = 0,
		.can_leave = &can_leave_east,
		.can_enter = &can_leave_west,
		.idle_anim = &character_base_male_east_idle,
		.walking_anim = &character_base_male_east_walking,
	},
	[DIRECTION_WEST] = {
		.dx = -1,
		.dy = 0,
		.can_leave = &can_leave_west,
		.can_enter = &can_leave_east,
		.idle_anim = &character_base_male_west_idle,
		.walking_anim = &character_base_male_west_walking,
	},
};

static bool can_move_in_direction(tile_coord_t from, enum direction direction) {
	const struct direction_info* direction_info = &direction_infos[direction];

	enum WalkaroundBehavior current_behavior = metatile_at(walkaround_state.map, from.x, from.y)->behavior;
	enum WalkaroundBehavior target_behavior = metatile_at(walkaround_state.map, from.x + direction_info->dx, from.y + direction_info->dy)->behavior;

	return direction_info->can_leave(current_behavior) && direction_info->can_enter(target_behavior);
}

static bool player_switch_or_advance_anim(const struct oam_animation* target_anim) {
	if (walkaround_viewmodel.player.anim == target_anim) {
		walkaround_viewmodel.player.anim_delay -= 1;
		if (0 == walkaround_viewmodel.player.anim_delay) {
			walkaround_viewmodel.player.anim_frame += 1;
			if (OAM_INDEX_GOTO == walkaround_viewmodel.player.anim->frames[
						walkaround_viewmodel.player.anim_frame].oam_index) {
				walkaround_viewmodel.player.anim_frame = walkaround_viewmodel.player.anim->frames[walkaround_viewmodel.player.anim_frame].delay;
			}
			walkaround_viewmodel.player.anim_delay = walkaround_viewmodel.player.anim->frames[walkaround_viewmodel.player.anim_frame].delay;
			return true;
		}
	} else {
		walkaround_viewmodel.player.anim = target_anim;
		walkaround_viewmodel.player.anim_frame = 0;
		walkaround_viewmodel.player.anim_delay = walkaround_viewmodel.player.anim->frames[0].delay;
		return true;
	}
	return false;
}

/*
 * @return whether the player sprite was moved or changed
 */
static bool normal_player_movement(void) {
	mapoffs_t target_mapoffs = player_tile_coord_to_target_pixel_coord(walkaround_state.player.pos);
	if (walkaround_viewmodel.player.mapoffs.x != target_mapoffs.x || walkaround_viewmodel.player.mapoffs.y != target_mapoffs.y) {
		if (walkaround_viewmodel.player.mapoffs.x < target_mapoffs.x) {
			walkaround_viewmodel.player.mapoffs.x += PLAYER_MOVE_SPEED;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (walkaround_viewmodel.player.mapoffs.x > target_mapoffs.x) {
			walkaround_viewmodel.player.mapoffs.x -= PLAYER_MOVE_SPEED;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (walkaround_viewmodel.player.mapoffs.y < target_mapoffs.y) {
			walkaround_viewmodel.player.mapoffs.y += PLAYER_MOVE_SPEED;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (walkaround_viewmodel.player.mapoffs.y > target_mapoffs.y) {
			walkaround_viewmodel.player.mapoffs.y -= PLAYER_MOVE_SPEED;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
	} else if (walkaround_state.player.action == ACTION_TURNING) {
		walkaround_state.player.turn_timer -= 1;
		if (0 == walkaround_state.player.turn_timer) {
			walkaround_state.player.action = ACTION_NONE;
		}
		return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
	} else {
		keypad_t inputs = keyinput_get_down();

		// if holding down both a horizontal direction and a vertical direction,
		// the `(walkaround_state.player.pos.x % 2 == walkaround_state.player.pos.y % 2)` part will make the player alternate between moving horizontally and vertically
		if (! inputs.right && (walkaround_state.player.pos.x % 2 == walkaround_state.player.pos.y % 2) && can_move_in_direction(walkaround_state.player.pos, DIRECTION_EAST)) {
			if (walkaround_state.player.action == ACTION_WALKING || walkaround_state.player.facing == DIRECTION_EAST) {
				walkaround_state.player.pos.x += 1;
				walkaround_viewmodel.player.mapoffs.x += PLAYER_MOVE_SPEED;
				walkaround_state.player.action = ACTION_WALKING;
			} else {
				walkaround_state.player.turn_timer = TURN_FRAMES;
				walkaround_state.player.action = ACTION_TURNING;
			}
			walkaround_state.player.facing = DIRECTION_EAST;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.left && (walkaround_state.player.pos.x % 2 == walkaround_state.player.pos.y % 2) && can_move_in_direction(walkaround_state.player.pos, DIRECTION_WEST)) {
			if (walkaround_state.player.action == ACTION_WALKING || walkaround_state.player.facing == DIRECTION_WEST) {
				walkaround_state.player.pos.x -= 1;
				walkaround_viewmodel.player.mapoffs.x -= PLAYER_MOVE_SPEED;
				walkaround_state.player.action = ACTION_WALKING;
			} else {
				walkaround_state.player.turn_timer = TURN_FRAMES;
				walkaround_state.player.action = ACTION_TURNING;
			}
			walkaround_state.player.facing = DIRECTION_WEST;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.up && can_move_in_direction(walkaround_state.player.pos, DIRECTION_NORTH)) {
			if (walkaround_state.player.action == ACTION_WALKING || walkaround_state.player.facing == DIRECTION_NORTH) {
				walkaround_state.player.pos.y -= 1;
				walkaround_viewmodel.player.mapoffs.y -= PLAYER_MOVE_SPEED;
				walkaround_state.player.action = ACTION_WALKING;
			} else {
				walkaround_state.player.turn_timer = TURN_FRAMES;
				walkaround_state.player.action = ACTION_TURNING;
			}
			walkaround_state.player.facing = DIRECTION_NORTH;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.down && can_move_in_direction(walkaround_state.player.pos, DIRECTION_SOUTH)) {
			if (walkaround_state.player.action == ACTION_WALKING || walkaround_state.player.facing == DIRECTION_SOUTH) {
				walkaround_state.player.pos.y += 1;
				walkaround_viewmodel.player.mapoffs.y += PLAYER_MOVE_SPEED;
				walkaround_state.player.action = ACTION_WALKING;
			} else {
				walkaround_state.player.turn_timer = TURN_FRAMES;
				walkaround_state.player.action = ACTION_TURNING;
			}
			walkaround_state.player.facing = DIRECTION_SOUTH;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.right && can_move_in_direction(walkaround_state.player.pos, DIRECTION_EAST)) {
			if (walkaround_state.player.action == ACTION_WALKING || walkaround_state.player.facing == DIRECTION_EAST) {
				walkaround_state.player.pos.x += 1;
				walkaround_viewmodel.player.mapoffs.x += PLAYER_MOVE_SPEED;
				walkaround_state.player.action = ACTION_WALKING;
			} else {
				walkaround_state.player.turn_timer = TURN_FRAMES;
				walkaround_state.player.action = ACTION_TURNING;
			}
			walkaround_state.player.facing = DIRECTION_EAST;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.left && can_move_in_direction(walkaround_state.player.pos, DIRECTION_WEST)) {
			if (walkaround_state.player.action == ACTION_WALKING || walkaround_state.player.facing == DIRECTION_WEST) {
				walkaround_state.player.pos.x -= 1;
				walkaround_viewmodel.player.mapoffs.x -= PLAYER_MOVE_SPEED;
				walkaround_state.player.action = ACTION_WALKING;
			} else {
				walkaround_state.player.turn_timer = TURN_FRAMES;
				walkaround_state.player.action = ACTION_TURNING;
			}
			walkaround_state.player.facing = DIRECTION_WEST;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.right) {
			walkaround_state.player.facing = DIRECTION_EAST;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.left) {
			walkaround_state.player.facing = DIRECTION_WEST;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.up) {
			walkaround_state.player.facing = DIRECTION_NORTH;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}
		else if (! inputs.down) {
			walkaround_state.player.facing = DIRECTION_SOUTH;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].walking_anim);
		}

		else {
			walkaround_state.player.action = ACTION_NONE;
			return player_switch_or_advance_anim(direction_infos[walkaround_state.player.facing].idle_anim);
		}
	}

	return false;
}

void MainCB_walkaround_main(void) {
	bool refresh_player = false;

	refresh_player |= normal_player_movement();

	refresh_player |= move_camera_towards(center_player_in_camera_target(), 2);

	if (refresh_player) {
		shadow_oam_rewrite_sprite(
			walkaround_viewmodel.player.oam_id,
			walkaround_viewmodel.player.anim->oams[
				walkaround_viewmodel.player.anim->frames[
					walkaround_viewmodel.player.anim_frame].oam_index],
			player_oam_position(walkaround_viewmodel.player.mapoffs));
	}
}
