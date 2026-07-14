#include "scene/isometric.h"

#include <stdlib.h>
#include "management/shadow_oam.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "utils/arraycount.h"
#include "graphics.h"

static union palette512 InitFadeIn_isometric(void);
void MainCB_isometric(void);

__attribute__((section(".sbss")))
struct {
} viewmodel_isometric = {};

__attribute__((section(".sbss")))
struct {
} state_isometric = {};

enum {
	BELOW_CHARBLOCK = 0,
	ABOVE_CHARBLOCK = 2,

	BELOW_ODDS_SCREENBLOCK = 28,
	BELOW_EVENS_SCREENBLOCK = 29,
	ABOVE_ODDS_SCREENBLOCK = 30,
	ABOVE_EVENS_SCREENBLOCK = 31,
};

static const struct shadow_vram_init shadow_vram_init__isometric = {
	.enable_bg = {true, true, true, true},
	.enable_obj = false,
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

static const unsigned terrain[10][10] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 3, 3, 3, 3, 3, 3, 3, 3, 1},
	{1, 3, 3, 3, 3, 3, 3, 3, 3, 1},
	{1, 3, 3, 3, 3, 3, 3, 3, 3, 1},
	{1, 3, 3, 3, 3, 3, 3, 3, 3, 1},
	{1, 3, 3, 3, 3, 6, 3, 3, 3, 1},
	{1, 3, 3, 3, 3, 3, 3, 6, 3, 1},
	{1, 3, 3, 3, 3, 3, 6, 6, 3, 1},
	{1, 3, 3, 3, 3, 3, 3, 3, 3, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
};

static union palette512 InitFadeIn_isometric(void) {
	union palette512 final_palette = {0};
	shadow_vram_init(&shadow_vram_init__isometric);
	shadow_oam_init();

	shadow_tiles_load_tileset_fixed_no_palette_vram_op(
		&final_palette,
		&isometric_isometric,
		(shadow_tiles_load_tileset_fixed_t) {
			.bg = 2, .start_palette = 0, .start_tiles = 0});
	final_palette.background._8[0] = rgb(0,0,0);

	bg_tile_t* screens = malloc(4 * sizeof(screenblock_t));
	if (screens) {
		CpuFastFill(0, screens, 4 * sizeof(screenblock_t) / sizeof(uint32_t));

		for (unsigned y = 0; y < arraycount(terrain); y++)
		for (unsigned x = 0; x < arraycount(terrain[0]); x++)
		{
			unsigned polarity = (x + y) % 2;
			unsigned screen_y = 4 + (x + y) / 2;
			unsigned screen_x = 14 + x - y;

			unsigned position_in_layer = screen_y * 32 + screen_x;
			unsigned position_of_below_layer = (polarity) * sizeof(screenblock_t) / sizeof(bg_tile_t);
			unsigned position_below = position_of_below_layer + position_in_layer;

			screens[position_below] = (bg_tile_t) {terrain[y][x] * 2, false, false, 0};
			screens[position_below + 1] = (bg_tile_t) {terrain[y][x] * 2 + 1, false, false, 0};
		}
		vram_op_queue_enqueue(&(struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = screens,
				.to_block = BELOW_ODDS_SCREENBLOCK,
				.to_tile = 0,
				.count = 4 * sizeof(screenblock_t) / sizeof(bg_tile_t),
			},
		});
	}

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


	return final_palette;
}

void MainCB_isometric(void) {
}
