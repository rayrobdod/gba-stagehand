// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Antonio Niño Díaz, 2022
// SPDX-FileContributor: Raymond Dodge, 2025

#include "main.h"

#include "gba/bios.h"
#include "management/isr.h"
#include "management/keyinput.h"
#include "management/shadow_oam.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "mgba.h"

MainCallback scene_onframe_callback;

int main(int argc, char* argv[]) {
	isr_switchboard_init();
	isr_enable(II_VBLANK);

	shadow_oam_init();

	MgbaOpen();

	scene_onframe_callback = &MainCB_mainMenu_init;

	while (1) {
		if (reg_lcd.VCOUNT < 160 || reg_lcd.VCOUNT > 175) {
			MgbaPrintf(MGBA_LOG_INFO, "VCOUNT: %d", reg_lcd.VCOUNT);
		}
		VBlankIntrWait();

		vram_op_queue_execute();
		keyinput_read();

		if (scene_onframe_callback)
			scene_onframe_callback();
	}

	return 0;
}
