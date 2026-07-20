#include "scene/mode3.h"

#include <stddef.h>
#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/vram.h"
#include "management/isr.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "transition/star_iris.h"
#include "graphics.h"
#include "mgba.h"

static union palette512 InitFadeIn_mode3(void);
static void MainCB_mode3(void);

static const struct transitionSourceCallbacks transitionSourceCbs_mode3 = {
	.fadeOut = NULL,
	.cleanup = NULL,
};
const struct transitionTargetCallbacks transitionTargetCbs_mode3 = {
	.initFadeOut = NULL,
	.fadeOut = NULL,
	.initFadeIn = InitFadeIn_mode3,
	.fadeIn = NULL,
	.target = MainCB_mode3,
};

static const struct reg_dma dma_mountain = {
	.source = snow_mountain_under_stars,
	.destination = vram.mode3,
	.word_count = 240 * 160,
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

static const struct vram_op vramop_mode3 = {
	.type = VRAM_QUEUE_OP_HWREG_DISPCNT,
	.dispcnt = {
		.value = (dispcnt_t) {
			.mode = 3,
			.enable_bg2 = true,
		}
	},
};

static union palette512 InitFadeIn_mode3(void) {
	VBlankIntrWait();
	reg_lcd.DISPCNT = (dispcnt_t){0};
	reg_dma[3] = dma_mountain;
	MgbaPrintf(MGBA_LOG_INFO, "VCOUNT: %d", reg_lcd.VCOUNT);

	vram_op_queue_enqueue(&vramop_mode3);

	union palette512 retval;
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < 16; j++) {
			retval.background._4[i][j] = rgb(i * 31 / 15, j * 31 / 15, 16);
			retval.object._4[i][j] = rgb(i * 31 / 15, j * 31 / 15, 16);
		}
	}
	return retval;
}

static void MainCB_mode3(void) {
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

	StartTransition(
		&transition_starIris,
		&transitionSourceCbs_mode3,
		&transitionTargetCbs_mainMenu);
}
