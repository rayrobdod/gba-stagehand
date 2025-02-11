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
#include "gba/shared.h"
#include "gba/palette.h"
#include "gba/vram.h"
#include "isr.h"
#include "mgba.h"
#include "scene_graphics.h"
#include "gba/screen.h"
#include "oldschool.png.h"


/* Adds current and change, without overflowing the bounds of min and max */
static uint16_t saturating_add(uint16_t current, uint16_t min, uint16_t max, int16_t change) {
    if (0 == change)
        return current;
    else if (0 < change) {
        if (max - current < change) {
            return max;
        } else {
            return current + change;
        }
    } else {
        change = -change;
        if (current - min < change) {
            return min;
        } else {
            return current - change;
        }
    }
}

void triangle_window_isr(void) {
	if (reg_lcd.VCOUNT < DISPLAY_HEIGHT) {
		reg_lcd.WIN0H = (window_horizontal_t) {
			.right = 240 - reg_lcd.VCOUNT,
			.left = 0,
		};
	}
}


int main(int argc, char *argv[])
{
    reg_lcd.DISPCNT = (dispcnt_t) {
        .mode = 0,
        .enable_bg1 = true,
        .enable_win0 = true,
    };

    reg_lcd.BG1CNT = (bgcnt_t) {
        .priority = 0,
        .charblock = 0,
        .screenblock = 31,
    };

    reg_lcd.WINOUT = (window_enable_pair_t) {0};
    reg_lcd.WININ = (window_enable_pair_t) {WIN_ENABLE_ALL, WIN_ENABLE_ALL};
    reg_lcd.WIN0H = (window_horizontal_t) {240, 0};
    reg_lcd.WIN0V = (window_vertical_t) {160, 0};

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
    isr_enable(II_HBLANK);

    isr_switchboard_set(II_HBLANK, triangle_window_isr);

    rgb_t current_color = rgb(28, 28, 28);

    while(1) {
        VBlankIntrWait();

        reg_lcd.WIN0H = (window_horizontal_t) {.left = 8, .right = 240};

        if (! reg_keypad.KEYINPUT.left) {
            current_color.r = saturating_add(current_color.r, 0, 31, -1);
        }
        if (! reg_keypad.KEYINPUT.right) {
            current_color.r = saturating_add(current_color.r, 0, 31, 1);
        }

        background_palette[0][0] = current_color;
    }

    return 0;
}
