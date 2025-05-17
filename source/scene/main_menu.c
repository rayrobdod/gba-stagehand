#include "scene/main_menu.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/screen.h"
#include "management/keyinput.h"
#include "management/scene_graphics.h"
#include "management/shadow_oam.h"
#include "management/vram_op_queue.h"
#include "scene/brick_break.h"
#include "scene/mode3.h"
#include "graphics.h"
#include "main.h"
#include "mgba.h"
#include "saturating_add.h"

// model
static uint8_t selection = 0;

// viewmodel
static uint8_t arrow_wiggle_timer = 0xFF;
static shadow_oam_id_t spriteid_arrow = 0;

//
static const unsigned TILEMAP_BUFFER_COUNT = 32 * 20;

//
static void MainCB_mainMenu_main(void);

static void print_to_tilemap(bg_tile_t* buffer, unsigned x, unsigned y, char* message) {
	unsigned start_index = y * 32 + x;
	unsigned length = strlen(message);

	for (unsigned i = 0; i < length; i++) {
		buffer[start_index + i] = (bg_tile_t){message[i]};
	}
}

static const palette16_t mono_pal = {{31,31,31}, {0,0,0}};

void MainCB_mainMenu_init(void) {
	shadow_oam_free_all();

	vram_op_queue_enqueue((struct vram_op) {
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

	vram_op_queue_enqueue((struct vram_op) {
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

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_TILES_BITUNPACK,
		.tiles_bitunpack = {
			oldschool.data,
			0,
			' ',
			{
				.src_length = oldschool.size,
				.src_bitsize = 1,
				.dest_bitsize = 4,
			}
		}
	});
	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_PALETTES,
		.palettes = {
			&mono_pal,
			0,
			1,
		}
	});

	spriteid_arrow = shadow_oam_add_sprite(
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

		print_to_tilemap(tilemap_buffer, 3, 2, "Mode 3");
		print_to_tilemap(tilemap_buffer, 3, 3, "Brick Break");
		print_to_tilemap(tilemap_buffer, 3, 4, "Option 3");
		print_to_tilemap(tilemap_buffer, 3, 5, "Option 4");

		MgbaPrintf(MGBA_LOG_DEBUG, "sizeof(struct vram_op) = %d", sizeof(struct vram_op));

		vram_op_queue_enqueue((struct vram_op){
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

	scene_onframe_callback = &MainCB_mainMenu_main;
}

static void MainCB_mainMenu_main(void) {
	arrow_wiggle_timer++;

	if (! keyinput_get_new().a) {
		shadow_oam_free_all();

		switch (selection) {
		case 0:
			scene_onframe_callback = &MainCB_mode3_init;
			break;
		default:
			scene_onframe_callback = &MainCB_brickBreak_init;
			break;
		}
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
