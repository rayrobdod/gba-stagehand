#include "scene/isometric.h"

#include <stdlib.h>
#include "management/keyinput.h"
#include "management/shadow_oam.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "utils/arraycount.h"
#include "utils/saturating_add.h"
#include "graphics.h"
#include "graphics_types.h"

static union palette512 InitFadeIn_isometric(void);
void MainCB_isometric(void);

__attribute__((packed))
enum Terrain {
	TERRAIN_PLAIN,
	TERRAIN_WATER,
	TERRAIN_MEADOW,
	TERRAIN_ROAD,
	TERRAIN_BUILDING,
};

typedef struct {
	uint8_t x;
	uint8_t y;
} grid_coords_t;

typedef struct {
	uint8_t polarity;
	uint8_t x;
	uint8_t y;
} hwmap_coords_t;

__attribute__((section(".sbss")))
struct {
	bg_tile_t screens[4 * 0x400];
	struct {
		shadow_oam_id_t oamid;
		grid_coords_t position;
	} cursor;
} viewmodel_isometric = {};

enum {
	TERRAIN_HEIGHT = 10,
	TERRAIN_WIDTH = 10,
};

__attribute__((section(".sbss")))
struct {
	enum Terrain terrain[TERRAIN_HEIGHT][TERRAIN_WIDTH];
} state_isometric = {};

enum {
	BELOW_CHARBLOCK = 0,
	ABOVE_CHARBLOCK = 0,

	BELOW_ODDS_SCREENBLOCK = 28,
	BELOW_EVENS_SCREENBLOCK = 29,
	ABOVE_ODDS_SCREENBLOCK = 30,
	ABOVE_EVENS_SCREENBLOCK = 31,
};

static const struct shadow_vram_init shadow_vram_init__isometric = {
	.enable_bg = {true, true, true, true},
	.enable_obj = true,
	.bgcnt = {
		[0] = {
			.priority = 0,
			.charblock = ABOVE_CHARBLOCK,
			.screenblock = ABOVE_EVENS_SCREENBLOCK,
		},
		[1] = {
			.priority = 1,
			.charblock = ABOVE_CHARBLOCK,
			.screenblock = ABOVE_ODDS_SCREENBLOCK,
		},
		[2] = {
			.priority = 2,
			.charblock = BELOW_CHARBLOCK,
			.screenblock = BELOW_EVENS_SCREENBLOCK,
		},
		[3] = {
			.priority = 3,
			.charblock = BELOW_CHARBLOCK,
			.screenblock = BELOW_ODDS_SCREENBLOCK,
		}
	}
};


const struct transitionTargetCallbacks transitionTargetCbs_isometric = {
	.initFadeOut = NULL,
	.fadeOut = NULL,
	.initFadeIn = InitFadeIn_isometric,
	.fadeIn = NULL,
	.target = MainCB_isometric,
};

static hwmap_coords_t coords_grid_to_hwmap_2(unsigned x, unsigned y) {
	return (hwmap_coords_t) {
		.polarity = (x + y) % 2,
		.x = 14 + x - y,
		.y = 4 + (x + y) / 2,
	};
}
static hwmap_coords_t coords_grid_to_hwmap(grid_coords_t in) {
	return coords_grid_to_hwmap_2(in.x, in.y);
}
static ucoords16_t coords_hwmap_to_screen(hwmap_coords_t in) {
	return (ucoords16_t) {
		.x = in.x * 8,
		.y = in.y * 8 - (in.x % 2 ? 0 : 4),
	};
}

void render_one_terrain(unsigned grid_x, unsigned grid_y) {
	hwmap_coords_t screen = coords_grid_to_hwmap_2(grid_x, grid_y);

	unsigned position_in_layer = screen.y * 32 + screen.x;
	unsigned position_of_below_layer = (screen.polarity) * sizeof(screenblock_t) / sizeof(bg_tile_t);
	unsigned position_below = position_of_below_layer + position_in_layer;

	switch (state_isometric.terrain[grid_y][grid_x]) {
	case TERRAIN_PLAIN:
		viewmodel_isometric.screens[position_below] = isometric_plain.tiles[0];
		viewmodel_isometric.screens[position_below + 1] = isometric_plain.tiles[1];
		break;
	case TERRAIN_WATER:
		viewmodel_isometric.screens[position_below] = isometric_water.tiles[0];
		viewmodel_isometric.screens[position_below + 1] = isometric_water.tiles[1];
		break;
	case TERRAIN_MEADOW:
		viewmodel_isometric.screens[position_below] = isometric_meadow.tiles[0];
		viewmodel_isometric.screens[position_below + 1] = isometric_meadow.tiles[1];
		break;
	case TERRAIN_BUILDING:
		break;
	case TERRAIN_ROAD:
		unsigned metatileid = 0
			+ (grid_x > 0 && TERRAIN_ROAD == state_isometric.terrain[grid_y][grid_x - 1] ? 1 : 0)
			+ (grid_y > 0 && TERRAIN_ROAD == state_isometric.terrain[grid_y - 1][grid_x] ? 2 : 0)
			+ (grid_x + 1 < TERRAIN_WIDTH && TERRAIN_ROAD == state_isometric.terrain[grid_y][grid_x + 1] ? 4 : 0)
			+ (grid_y + 1 < TERRAIN_HEIGHT && TERRAIN_ROAD == state_isometric.terrain[grid_y + 1][grid_x] ? 8 : 0)
			;
		viewmodel_isometric.screens[position_below] = isometric_road.metatile[metatileid].tile[0];
		viewmodel_isometric.screens[position_below + 1] = isometric_road.metatile[metatileid].tile[1];
		break;
	}
}
void render_one_building(unsigned grid_x, unsigned grid_y) {
	hwmap_coords_t screen = coords_grid_to_hwmap_2(grid_x, grid_y);
	unsigned position_in_layer = screen.y * 32 + screen.x;
	unsigned position_of_layer = (screen.polarity) * sizeof(screenblock_t) / sizeof(bg_tile_t);
	unsigned position = position_of_layer + position_in_layer;
	viewmodel_isometric.screens[position] = isometric_cube1.tiles[0];
	viewmodel_isometric.screens[position + 1] = isometric_cube1.tiles[1];

	screen = coords_grid_to_hwmap_2(grid_x, grid_y - 1);
	position_in_layer = screen.y * 32 + screen.x;
	position_of_layer = (screen.polarity + 2) * sizeof(screenblock_t) / sizeof(bg_tile_t);
	position = position_of_layer + position_in_layer;
	viewmodel_isometric.screens[position] = isometric_cube1.tiles[3];

	screen = coords_grid_to_hwmap_2(grid_x - 1, grid_y);
	position_in_layer = screen.y * 32 + screen.x;
	position_of_layer = (screen.polarity + 2) * sizeof(screenblock_t) / sizeof(bg_tile_t);
	position = position_of_layer + position_in_layer;
	viewmodel_isometric.screens[position + 1] = isometric_cube1.tiles[2];

	screen = coords_grid_to_hwmap_2(grid_x - 1, grid_y - 1);
	position_in_layer = screen.y * 32 + screen.x;
	position_of_layer = (screen.polarity + 2) * sizeof(screenblock_t) / sizeof(bg_tile_t);
	position = position_of_layer + position_in_layer;
	viewmodel_isometric.screens[position] = isometric_cube1.tiles[4];
	viewmodel_isometric.screens[position + 1] = isometric_cube1.tiles[5];
}

static union palette512 InitFadeIn_isometric(void) {
	union palette512 final_palette = {0};
	state_isometric = (typeof(state_isometric)) {};
	viewmodel_isometric = (typeof(viewmodel_isometric)) {};

	shadow_vram_init(&shadow_vram_init__isometric);
	shadow_oam_init();

	shadow_tiles_load_tileset_fixed_no_palette_vram_op(
		&final_palette,
		&isometric_city_tileset,
		(shadow_tiles_load_tileset_fixed_t) {
			.bg = 2, .start_palette = 0, .start_tiles = 0});
	final_palette.background._8[0] = rgb(0,0,0);

	for (unsigned x = 2; x < 8; x++) {
		state_isometric.terrain[0][x] = TERRAIN_WATER;
		state_isometric.terrain[8][x] = TERRAIN_MEADOW;
	}
	state_isometric.terrain[4][1] = TERRAIN_BUILDING;
	state_isometric.terrain[4][6] = TERRAIN_BUILDING;
	state_isometric.terrain[4][7] = TERRAIN_BUILDING;
	state_isometric.terrain[5][6] = TERRAIN_BUILDING;
	state_isometric.terrain[5][7] = TERRAIN_BUILDING;

	for (unsigned y = 0; y < TERRAIN_HEIGHT; y++)
	for (unsigned x = 0; x < TERRAIN_WIDTH; x++)
	{
		render_one_terrain(x, y);
	}

	render_one_building(1,4);

	render_one_building(6,4);
	render_one_building(7,4);
	render_one_building(6,5);
	render_one_building(7,5);

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP,
		.map_free = {
			.from = viewmodel_isometric.screens,
			.to_block = BELOW_ODDS_SCREENBLOCK,
			.to_tile = 0,
			.count = 4 * sizeof(screenblock_t) / sizeof(bg_tile_t),
		},
	});

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {
				{.h = 0, .v = 0},
				{.h = 0, .v = 4},
				{.h = 0, .v = 0},
				{.h = 0, .v = 4},
			},
		},
	});

	viewmodel_isometric.cursor.position.x = 0;
	viewmodel_isometric.cursor.position.y = 0;
	viewmodel_isometric.cursor.oamid = shadow_oam_add_sprite_no_palette_vram_op(
		&final_palette,
		&isometric_cursor,
		(struct shadow_oam_position) {
			coords_hwmap_to_screen(coords_grid_to_hwmap(viewmodel_isometric.cursor.position)),
			HOTSPOT_TOPLEFT,
			false,
			false,
			0,
		}
	);

	return final_palette;
}

void MainCB_isometric(void) {
	viewmodel_isometric.cursor.position.x = saturating_add(
			viewmodel_isometric.cursor.position.x,
			0, TERRAIN_WIDTH - 1, keyinput_horizontal_new());
	viewmodel_isometric.cursor.position.y = saturating_add(
			viewmodel_isometric.cursor.position.y,
			0, TERRAIN_HEIGHT - 1, keyinput_vertical_new());
	shadow_oam_move_sprite(
		viewmodel_isometric.cursor.oamid,
		(struct shadow_oam_position) {
			coords_hwmap_to_screen(coords_grid_to_hwmap(viewmodel_isometric.cursor.position)),
			HOTSPOT_TOPLEFT,
			false,
			false,
			0,
		}
	);

	if (!keyinput_get_new().a) {
		if (TERRAIN_PLAIN == state_isometric.terrain[viewmodel_isometric.cursor.position.y][viewmodel_isometric.cursor.position.x]) {
			state_isometric.terrain[viewmodel_isometric.cursor.position.y][viewmodel_isometric.cursor.position.x] = TERRAIN_ROAD;
		}
		else if (TERRAIN_ROAD == state_isometric.terrain[viewmodel_isometric.cursor.position.y][viewmodel_isometric.cursor.position.x]) {
			state_isometric.terrain[viewmodel_isometric.cursor.position.y][viewmodel_isometric.cursor.position.x] = TERRAIN_PLAIN;
		}

		render_one_terrain(viewmodel_isometric.cursor.position.x, viewmodel_isometric.cursor.position.y);
		if (viewmodel_isometric.cursor.position.x > 0)
			render_one_terrain(viewmodel_isometric.cursor.position.x - 1, viewmodel_isometric.cursor.position.y);
		if (viewmodel_isometric.cursor.position.x + 1 < TERRAIN_WIDTH)
			render_one_terrain(viewmodel_isometric.cursor.position.x + 1, viewmodel_isometric.cursor.position.y);
		if (viewmodel_isometric.cursor.position.y > 0)
			render_one_terrain(viewmodel_isometric.cursor.position.x, viewmodel_isometric.cursor.position.y - 1);
		if (viewmodel_isometric.cursor.position.y + 1 < TERRAIN_HEIGHT)
			render_one_terrain(viewmodel_isometric.cursor.position.x, viewmodel_isometric.cursor.position.y + 1);

		vram_op_queue_enqueue(&(struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP,
			.map_free = {
				.from = viewmodel_isometric.screens,
				.to_block = BELOW_ODDS_SCREENBLOCK,
				.to_tile = 0,
				.count = 4 * sizeof(screenblock_t) / sizeof(bg_tile_t),
			},
		});
	}
}
