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
#include "mgba.h"
#include "options.h"

MainCallback scene_onframe_callback;

int main() {
	reg_interrupt.WAITCNT = common_waitcnt;

	isr_switchboard_init();
	isr_enable(II_VBLANK);

	shadow_oam_init();

	MgbaOpen();
	options_load_from_save();

	scene_onframe_callback = initial_scene_onframe_callback;

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
