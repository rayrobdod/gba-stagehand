#include "scene/mode3.h"

#include <stdint.h>
#include <stdlib.h>
#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/hw_reg_cast.h"
#include "gba/vram.h"
#include "management/isr.h"
#include "scene/main_menu.h"
#include "graphics.h"
#include "main.h"
#include "mgba.h"

void MainCB_mode3_init(void) {

	reg_lcd.DISPCNT = (dispcnt_t){
		.mode = 3,
		.enable_bg2 = true,
	};

	VBlankIntrWait();
	CpuFastSet(
		snow_mountain_under_stars,
		vram.mode3,
		(struct CpuFastSet){
			.word_count = 240 / 2 * 80,
			.mode = CPU_SET_COPY,
		});
	MgbaPrintf(MGBA_LOG_INFO, "VCOUNT: %d", reg_lcd.VCOUNT);

	VBlankIntrWait();
	CpuFastSet(
		snow_mountain_under_stars[80],
		vram.mode3[80],
		(struct CpuFastSet){
			.word_count = 240 / 2 * 80,
			.mode = CPU_SET_COPY,
		});
	MgbaPrintf(MGBA_LOG_INFO, "VCOUNT: %d", reg_lcd.VCOUNT);

	reg_keypad.KEYCNT = (keypad_control_t){
		.b = true,
		.condition = KEYPAD_CONDITION_AND,
	};
	isr_disable(II_VBLANK);
	isr_enable(II_KEYPAD);

	// TODO: `Stop` bios function?
	IntrWait(true, (interrupt_flag_t){.keypad = true});

	isr_disable(II_KEYPAD);
	isr_enable(II_VBLANK);

	scene_onframe_callback = &MainCB_mainMenu_init;
}
