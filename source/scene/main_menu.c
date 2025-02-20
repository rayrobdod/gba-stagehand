#include "scene/main_menu.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/screen.h"
#include "management/keyinput.h"
#include "management/scene_graphics.h"
#include "management/shadow_oam.h"
#include "management/vram_op_queue.h"
#include "main.h"
#include "mgba.h"
#include "saturating_add.h"
#include "graphics.h"
#include "oldschool.png.h"
#include "scene/brick_break.h"

// model
static uint8_t selection = 0;

// viewmodel
static uint8_t arrow_wiggle_timer = 0xFF;
static shadow_oam_id_t spriteid_arrow = 0;

//
static const unsigned TILEMAP_BUFFER_COUNT = 32 * 20;
static bg_tile_t* tilemap_buffer = NULL;

//
static void MainCB_mainMenu_init1(void);
static void MainCB_mainMenu_main(void);

static void print_to_tilemap(bg_tile_t* buffer, unsigned x, unsigned y, char* message) {
	unsigned start_index = y * 32 + x;
	unsigned length = strlen(message);

	for (unsigned i = 0; i < length; i++) {
		buffer[start_index + i] = (bg_tile_t){message[i]};
	}
}

void MainCB_mainMenu_init(void) {
	shadow_oam_free_all();

	reg_lcd.DISPCNT = (dispcnt_t) {
		.mode = 0,
		.obj_character_mapping = OBJ_CHAR_MAP_1D,
		.enable_bg1 = true,
		.enable_obj = true,
	};

	reg_lcd.BG1CNT = (bgcnt_t) {
		.priority = 0,
		.charblock = 0,
		.screenblock = 31,
	};

	queue_load_tileset_graphics(
		&oldschool,
		(struct load_tileset_graphics) {
			.charblock = 0,
			.palette_offset = 0,
			.tile_offset = ' ',
		});

	spriteid_arrow = shadow_oam_add_sprite(
		&arrow_right,
		(struct shadow_oam_position) {
			.coord = (ucoords16_t) {.x = 0, .y = DISPLAY_HEIGHT},
			.hotspot = HOTSPOT_RIGHT,
		});

	tilemap_buffer = malloc(TILEMAP_BUFFER_COUNT * sizeof(bg_tile_t));

	unsigned i;
	for (i = 0; i < TILEMAP_BUFFER_COUNT; i++) {
		tilemap_buffer[i] = (bg_tile_t) {' '};
	}

	print_to_tilemap(tilemap_buffer, 3, 2, "Option 1");
	print_to_tilemap(tilemap_buffer, 3, 3, "Brick Break");
	print_to_tilemap(tilemap_buffer, 3, 4, "Option 3");
	print_to_tilemap(tilemap_buffer, 3, 5, "Option 4");

	MgbaPrintf(MGBA_LOG_DEBUG, "sizeof(struct vram_op) = %d", sizeof(struct vram_op));

	vram_op_queue_enqueue((struct vram_op){
		.type = VRAM_QUEUE_OP_BG_MAP,
		.map = {
			.from = tilemap_buffer,
			.to_block = 31,
			.to_tile = 0,
			.count = TILEMAP_BUFFER_COUNT,
		}});

	scene_onframe_callback = &MainCB_mainMenu_init1;
}

static void MainCB_mainMenu_init1(void) {
	free(tilemap_buffer);
	tilemap_buffer = NULL;
	scene_onframe_callback = &MainCB_mainMenu_main;
	MainCB_mainMenu_main();
}

static void MainCB_mainMenu_main(void) {
	arrow_wiggle_timer++;

	if (! keyinput_get_new().a) {
		scene_onframe_callback = &MainCB_brickBreak_init;
	}

	bool redraw_arrow = false;

	if (0 == (arrow_wiggle_timer & 0xF)) {
		redraw_arrow = true;
	}
	int dselection = keyinput_vertical_new();
	if (dselection) {
		redraw_arrow = true;
		selection = saturating_add(selection, 0, 3, dselection);
	}

	if (redraw_arrow) {
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
		});
	}
}
