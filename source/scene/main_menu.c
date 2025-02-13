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
#include "arrow_right.png.h"
#include "oldschool.png.h"

// model
static uint8_t selection = 0;

// viewmodel
static uint8_t arrow_wiggle_timer = 0xFF;
static shadow_oam_id_t spriteid_arrow = 0;

static void print_to_tilemap(unsigned x, unsigned y, char* message)
{
	unsigned start_index = y * 32 + x;
	unsigned length = strlen(message);

	for (unsigned i = 0; i < length; i++) {
		vram.screenblock[31][start_index + i] = (bg_tile_t) {message[i]};
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

	load_tileset_graphics(
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

	unsigned i;
	for (i = 0; i < 32 * 32; i++) {
		vram.screenblock[31][i] = (bg_tile_t) {' '};
	}

	print_to_tilemap(3, 2, "Option 1");
	print_to_tilemap(3, 3, "Option 2");
	print_to_tilemap(3, 4, "Option 3");
	print_to_tilemap(3, 5, "Option 4");

	MgbaPrintf(MGBA_LOG_DEBUG, "sizeof(struct vram_op) = %d", sizeof(struct vram_op));

	scene_onframe_callback = &MainCB_mainMenu_main;
}

void MainCB_mainMenu_main(void) {
	arrow_wiggle_timer++;

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
