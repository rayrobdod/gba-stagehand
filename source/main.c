// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2022
// SPDX-FileContributor: Raymond Dodge, 2025

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/bios.h"
#include "gba/bios_reg.h"
#include "gba/hw_reg.h"
#include "gba/palette.h"
#include "gba/screen.h"
#include "gba/shared.h"
#include "gba/vram.h"
#include "management/isr.h"
#include "management/keyinput.h"
#include "management/scene_graphics.h"
#include "management/shadow_oam.h"
#include "management/vram_op_queue.h"
#include "mgba.h"
#include "saturating_add.h"
#include "arrow_left.png.h"
#include "arrow_right.png.h"
#include "oldschool.png.h"

int main(int argc, char *argv[])
{
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

	char message[] = "Hello World";

	unsigned i;
	for (i = 0; i < sizeof(message); i++) {
		bg_tile_t new_tile = {
			.tile = message[i],
			.palette = 0,
		};
		vram.screenblock[31][i] = new_tile;
	}
	for (; i < 32 * 32; i++) {
		vram.screenblock[31][i] = (bg_tile_t) {' '};
	}

	isr_switchboard_init();
	isr_enable(II_VBLANK);

	shadow_oam_init();

	MgbaOpen();

	int16_t arrow_right_index = shadow_oam_add_sprite(
		&arrow_right,
		(struct shadow_oam_position) {
			.coord = (ucoords16_t) {.x = 20, .y = 20},
			.hotspot = HOTSPOT_RIGHT,
		});

	int16_t arrow_left_index = shadow_oam_add_sprite(
		&arrow_left,
		(struct shadow_oam_position) {
			.coord = (ucoords16_t) {.x = 20, .y = 20},
			.hotspot = HOTSPOT_LEFT,
		});

	uint32_t arrow_wiggle_timer = 0;
	unsigned y = 20;
	unsigned rightX = 20;
	unsigned leftX = 20;

	while(1) {
		VBlankIntrWait();

		vram_op_queue_execute();
		keyinput_read();

		if (! keyinput_get_new().down) {
			y = saturating_add(y, 4, 84, 8);
		}
		if (! keyinput_get_new().up) {
			y = saturating_add(y, 4, 84, -8);
		}

		arrow_wiggle_timer++;
		if ((arrow_wiggle_timer & 0xF) == 0) {
			switch ((arrow_wiggle_timer & 0x30) >> 4) {
			case 0:
			case 1:
				rightX++;
				leftX--;
				break;
			case 2:
			case 3:
				rightX--;
				leftX++;
				break;
			}
		}

		shadow_oam_move_sprite(
			arrow_left_index,
			(struct shadow_oam_position) {
				.coord = (ucoords16_t) {.x = leftX, .y = y},
				.hotspot = HOTSPOT_LEFT,
		});
		shadow_oam_move_sprite(
			arrow_right_index,
			(struct shadow_oam_position) {
				.coord = (ucoords16_t) {.x = rightX, .y = y},
				.hotspot = HOTSPOT_RIGHT,
		});
	}

	return 0;
}
