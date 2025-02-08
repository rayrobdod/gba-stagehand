// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2022
// SPDX-FileContributor: Raymond Dodge, 2025

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/shared.h"
#include "gba/palette.h"
#include "gba/vram.h"
#include "oldschool.png.h"

int main(int argc, char *argv[])
{
    dispcnt_t newdispcnt = {
        .mode = 0,
        .enable_bg1 = true,
    };
    reg_lcd.DISPCNT = newdispcnt;

    bgcnt_t newbg1cnt = {
        .priority = 0,
        .charblock = 0,
        .screenblock = 31,
    };
    reg_lcd.BG1CNT = newbg1cnt;

    struct CpuFastSet cpuset = {
        .word_count = sizeof(oldschool) / 4,
        .mode = CPU_SET_COPY,
    };
    CpuFastSet(oldschool, &vram.bg_charblock[0][' '], cpuset);

    char message[] = "Hello World";

    for (int i = 0; i < sizeof(message); i++) {
        bg_tile_t new_tile = {
            .tile = message[i],
            .palette = 0,
        };
        vram.screenblock[31][i] = new_tile;
    }

    background_palette[0][0] = rgb(31, 31, 31);
    background_palette[0][1] = rgb(0, 0, 0);

    while(1);

    return 0;
}
