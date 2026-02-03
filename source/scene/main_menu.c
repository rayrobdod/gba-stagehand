#include "scene/main_menu.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/screen.h"
#include "management/keyinput.h"
#include "management/shadow_oam.h"
#include "management/transition.h"
#include "management/vram_op_queue.h"
#include "scene/brick_break.h"
#include "scene/dmg_music_using_notation.h"
#include "scene/display_credits.h"
#include "scene/gradient.h"
#include "scene/mode3.h"
#include "scene/options_menu.h"
#include "scene/parallax_mountain_dusk.h"
#include "scene/text_print_profile.h"
#include "scene/text_print_step.h"
#include "scene/walkaround.h"
#include "transition/palette_fade.h"
#include "utils/arraycount.h"
#include "utils/saturating_add.h"
#include "graphics.h"
#include "graphics_types.h"
#include "main.h"
#include "mgba.h"

union palette512 InitFadeIn_mainMenu(void);
static void MainCB_mainMenu_main(void);
static void MainCB_load(void);
static void FadeCB_mainMenu(void);
static void ChangeScene_options_for_mainmenu(void (*fadeCb)(void));

//

const MainCallback initial_scene_onframe_callback = MainCB_load;

// model
static uint8_t selection = 0;

// viewmodel
static uint8_t arrow_wiggle_timer = 0xFF;
static shadow_oam_id_t spriteid_arrow = 0;

//
static const unsigned TILEMAP_BUFFER_COUNT = 32 * 20;

static const struct transitionSourceCallbacks transitionSourceCbs_mainMenu = {
	.fadeOut = FadeCB_mainMenu,
	.cleanup = NULL,
};
const struct transitionTargetCallbacks transitionTargetCbs_mainMenu = {
	.initFadeOut = NULL,
	.fadeOut = NULL,
	.initFadeIn = InitFadeIn_mainMenu,
	.fadeIn = FadeCB_mainMenu,
	.target = MainCB_mainMenu_main,
};

static const struct {
	char* label;
	MainCallback cb;
	void (*startFn)(void (*fadeCb)(void));
	const struct transition* transition;
	const struct transitionTargetCallbacks* transitionCbs;
} menu_options[] = {
	{
		.label = "Brick Break",
		.transition = &transition_paletteFade_black,
		.transitionCbs = &transitionTargetCbs_brickBreak,
	},
	{
		.label = "Walkaround",
		.startFn = &ChangeScene_walkaround_newgame,
	},
	{
		.label = "Mode 3",
		.cb = &MainCB_mode3_init,
	},
	{
		.label = "Text Print Profile",
		.cb = &MainCB_textPrintProfile_init,
	},
	{
		.label = "Text Print Step",
		.transition = &transition_paletteFade_dodgerblue,
		.transitionCbs = &transitionTargetCbs_textPrintStep,
	},
	{
		.label = "Parallax Mountain Dusk",
		.transition = &transition_paletteFade__21_13_17,
		.transitionCbs = &transitionTargetCbs_parallaxMountainDusk,
	},
	{
		.label = "DMG Music",
		.transition = &transition_paletteFade_black,
		.transitionCbs = &transitionTargetCbs_dmgMusicUsingNotation,
	},
	{
		.label = "Gradient",
		.cb = &MainCB_gradient_init,
	},
	{
		.label = "Options",
		.startFn = &ChangeScene_options_for_mainmenu,
	},
	{
		.label = "Credits",
		.cb = &MainCB_credits_init,
	},
};

static void print_to_tilemap(bg_tile_t* buffer, unsigned x, unsigned y, char* message) {
	unsigned start_index = y * 32 + x;
	unsigned length = strlen(message);

	for (unsigned i = 0; i < length; i++) {
		buffer[start_index + i] = (bg_tile_t){message[i]};
	}
}

static void ChangeScene_mainmenu([[maybe_unused]] void (*_fadeCb)(void)) {
	scene_onframe_callback = &MainCB_mainMenu_init;
}

static void ChangeScene_options_for_mainmenu(void (*fadeCb)(void)) {
	ChangeScene_options(fadeCb, &ChangeScene_mainmenu);
}

static void MainCB_load(void) {
	StartTransition(
		&transition_cut,
		&(struct transitionSourceCallbacks) {0},
		&transitionTargetCbs_mainMenu);
}

union palette512 InitFadeIn_mainMenu(void) {
	union palette512 retval = {0};
	retval.background._4[0][0] = rgb(31,31,31);

	shadow_oam_free_all();

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_DISPCNT,
		.dispcnt = {
			.value = (dispcnt_t) {
				.mode = 0,
				.obj_character_mapping = OBJ_CHAR_MAP_1D,
				.enable_bg1 = true,
				.enable_obj = true,
			}
		},
	});

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGOFSS,
		.bgofss = {
			.value = {{0},{0},{0},{0},},
		},
	});

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_BGCNT,
		.bgcnt = {
			.value = (bgcnt_t) {
				.priority = 0,
				.charblock = 0,
				.screenblock = 31,
			},
			.to_index = 1
		}
	});

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES_BITUNPACK,
		.tiles_bitunpack = {
			oldschool.data,
			0,
			' ',
			{
				.src_length = oldschool.size,
				.src_bitsize = oldschool.unit_width,
				.dest_bitsize = 4,
			}
		}
	});

	spriteid_arrow = shadow_oam_add_sprite_no_palette_vram_op(
		&retval,
		&arrow_right,
		(struct shadow_oam_position) {
			.coord = (ucoords16_t) {.x = 0, .y = DISPLAY_HEIGHT},
			.hotspot = HOTSPOT_RIGHT,
		});

	bg_tile_t* tilemap_buffer = malloc(TILEMAP_BUFFER_COUNT * sizeof(bg_tile_t));
	if (tilemap_buffer) {
		unsigned i;
		for (i = 0; i < TILEMAP_BUFFER_COUNT; i++) {
			tilemap_buffer[i] = (bg_tile_t) {' '};
		}

		for (unsigned i = 0; i < arraycount(menu_options); i++) {
			if (menu_options[i].label) {
				print_to_tilemap(tilemap_buffer, 3, i + 2, menu_options[i].label);
			}
		}

		MgbaPrintf(MGBA_LOG_DEBUG, "sizeof(struct vram_op) = %d", sizeof(struct vram_op));

		vram_op_queue_enqueue(&(struct vram_op){
			.type = VRAM_QUEUE_OP_BG_MAP_FREE,
			.map_free = {
				.from = tilemap_buffer,
				.to_block = 31,
				.to_tile = 0,
				.count = TILEMAP_BUFFER_COUNT,
			}});
	} else {
		MgbaPrintf(MGBA_LOG_ERROR, "Did not allocate main_menu's tilemap_buffer");
	}

	return retval;
}

static void redraw_arrow(void) {
	uint16_t arrow_wiggle = 0;
	switch ((arrow_wiggle_timer & 0x30) >> 4) {
	case 1:
		arrow_wiggle = -1;
		break;
	case 3:
		arrow_wiggle = 1;
		break;
	}

	shadow_oam_move_sprite(
		spriteid_arrow,
		(struct shadow_oam_position) {
			.coord = (ucoords16_t) {
				.x = 24 + arrow_wiggle,
				.y = 20 + selection * 8,
			},
			.hotspot = HOTSPOT_RIGHT,
		}
	);
}

static void MainCB_mainMenu_main(void) {
	arrow_wiggle_timer++;

	if (! keyinput_get_new().a) {
		if (selection < arraycount(menu_options)) {
			if (menu_options[selection].transition && menu_options[selection].transitionCbs) {
			StartTransition(
				menu_options[selection].transition,
				&transitionSourceCbs_mainMenu,
				menu_options[selection].transitionCbs);
			} else
			if (menu_options[selection].startFn) {
				menu_options[selection].startFn(FadeCB_mainMenu);
			} else
			if (menu_options[selection].cb) {
				scene_onframe_callback = menu_options[selection].cb;
			}
		}
	}

	bool should_redraw_arrow = false;

	if (0 == (arrow_wiggle_timer & 0xF)) {
		should_redraw_arrow = true;
	}
	int dselection = keyinput_vertical_new();
	if (dselection) {
		should_redraw_arrow = true;
		selection = saturating_add(selection, 0, arraycount(menu_options) - 1, dselection);
	}

	if (should_redraw_arrow) {
		redraw_arrow();
	}
}

static void FadeCB_mainMenu(void) {
	arrow_wiggle_timer++;

	if (0 == (arrow_wiggle_timer & 0xF)) {
		redraw_arrow();
	}
}
