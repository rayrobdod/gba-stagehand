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

__attribute__((section(".sbss")))
static void (*fadeCb)(void) = {0};

__attribute__((section(".sbss")))
static struct {
	shadow_oam_id_t id;
	uint8_t x;
	uint8_t y;
} player = {0};

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

	shadow_tiles_load_tileset(&mushroom_village.tileset, (struct shadow_tiles_load_tileset) {.bg = 1});
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

	bg_tile_t* terrain = malloc(3 * sizeof(screenblock_t));
	if (terrain) {
		memset(terrain, 0, 3 * sizeof(screenblock_t));
		for (unsigned metay = 0; metay < 10; metay++)
		for (unsigned metax = 0; metax < 15; metax++)
		{
			unsigned metatile_id = mushroom_village.metatilemap[metax + mushroom_village.width * metay];
			unsigned position_in_layer = metay * 32 * 2 + metax * 2;

			for (unsigned layer = 0; layer < 3; ++layer) {
				unsigned position_of_layer = layer * sizeof(screenblock_t) / sizeof(bg_tile_t);
				unsigned position = position_of_layer + position_in_layer;

				terrain[position] = mushroom_village.metatileset[metatile_id].tiles[layer][0];
				terrain[position + 1] = mushroom_village.metatileset[metatile_id].tiles[layer][1];
				terrain[position + 32] = mushroom_village.metatileset[metatile_id].tiles[layer][2];
				terrain[position + 33] = mushroom_village.metatileset[metatile_id].tiles[layer][3];
			}
		}

		vram_op_queue_enqueue((struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = terrain,
				.to_block = TERRAIN_SCREENBLOCK,
				.to_tile = 0,
				.count = 3 * sizeof(screenblock_t),
			},
		});
	}

	struct shadow_oam_add_sprite_no_palette_vram_op player_oam =
		shadow_oam_add_sprite_no_palette_vram_op(
			&character_base_male_west_idle1,
			(struct shadow_oam_position) {
				.coord = {.x = 160-16-16-8, .y = 80 + 16 + 8 + 4 + 16},
				.hotspot = HOTSPOT_BOTTOM,
				.hflip = false,
				.vflip = false,
				.priority = 2,
			});
	player.id = player_oam.sprite_index;
	player.x = 160-16-16-8;
	player.y = 80 + 16 + 8 + 4 + 16;

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {{0},{0},{0},{0},},
		},
	});

	union palette512 final_palette = {0};
	memcpy(final_palette.background._4[0], mushroom_village.tileset.palette, 16 * 2 * 12);
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

static void MainCB_walkaround_main(void) {
	keypad_t inputs = keyinput_get_new();

	if (! inputs.left || ! inputs.right || ! inputs.up || ! inputs.down) {
		if (! inputs.left) player.x -= 16;
		if (! inputs.right) player.x += 16;
		if (! inputs.up) player.y -= 16;
		if (! inputs.down) player.y += 16;

		shadow_oam_move_sprite(player.id,
			(struct shadow_oam_position) {
				.coord = {.x = player.x, .y = player.y},
				.hotspot = HOTSPOT_BOTTOM,
				.hflip = false,
				.vflip = false,
				.priority = 2,
			});
	}
}
