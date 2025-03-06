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

static const struct reg_dma dma_mountain_0 = {
	.source = snow_mountain_under_stars,
	.destination = vram.mode3,
	.word_count = 240 * 80,
	.control = {
		.dest_control = DMA_ADDR_INCREMENT,
		.src_control = DMA_ADDR_INCREMENT,
		.repeat = false,
		.word_size = WORDSIZE_16BIT,
		.start = DMA_START_IMMEDIATELY,
		.irq = false,
		.enable = true,
	},
};
static const struct reg_dma dma_mountain_1 = {
	.source = snow_mountain_under_stars[80],
	.destination = vram.mode3[80],
	.word_count = 240 * 80,
	.control = {
		.dest_control = DMA_ADDR_INCREMENT,
		.src_control = DMA_ADDR_INCREMENT,
		.repeat = false,
		.word_size = WORDSIZE_16BIT,
		.start = DMA_START_IMMEDIATELY,
		.irq = false,
		.enable = true,
	},
};

void MainCB_mode3_init(void) {

	reg_lcd.DISPCNT = (dispcnt_t){
		.mode = 3,
		.enable_bg2 = true,
	};

	// According to the debug print, the gba is able to write half the data during a vblank
	// mGBA seems fine with writing the whole data in one frame,
	// even though by my understanding the gba should not be willing to write to VRAM when not in vblank

	if (true) {
		VBlankIntrWait();
		reg_dma[3] = dma_mountain_0;
		MgbaPrintf(MGBA_LOG_INFO, "VCOUNT: %d", reg_lcd.VCOUNT);
		VBlankIntrWait();
		reg_dma[3] = dma_mountain_1;
		MgbaPrintf(MGBA_LOG_INFO, "VCOUNT: %d", reg_lcd.VCOUNT);
	} else {
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
	}

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
