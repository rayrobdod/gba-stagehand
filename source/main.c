// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2022
// SPDX-FileContributor: Raymond Dodge, 2025

#include <stdint.h>
#include "gba/hw_reg.h"
#include "gba/shared.h"
#include "gba/vram.h"

int main(int argc, char *argv[])
{
    dispcnt_t newdispcnt = {
        .mode = 3,
        .enable_bg2 = true,
    };
    reg_lcd.DISPCNT = newdispcnt;

    vram.mode3[80][120] = rgb(31, 0, 0);
    vram.mode3[80][136] = rgb(0, 31, 0);
    vram.mode3[96][120] = rgb(0, 0, 31);

    while(1);

    return 0;
}
