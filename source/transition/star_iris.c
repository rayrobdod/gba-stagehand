#include "transition/star_iris.h"

#include <stdlib.h>
#include "gba/bios.h"
#include "gba/hw_reg.h"
#include "gba/screen.h"
#include "management/isr.h"
#include "management/vram_op_queue.h"
#include "mgba.h"

enum {
	FIRST_HALF_HEIGHT = 60,
};

static int16_t edgeX;
static int16_t edgeY;
static int16_t pivotY;

static window_horizontal_t window_scanline_effect[DISPLAY_HEIGHT];

static void HBlankCB_starIris(void) {
	if (reg_lcd.VCOUNT < DISPLAY_HEIGHT) {
		reg_lcd.WIN0H = window_scanline_effect[reg_lcd.VCOUNT];
	} else {
		reg_lcd.WIN0H = window_scanline_effect[0];
	}
}

static void transition_starIris_initOut(void) {
	edgeX = 0;
	edgeY = 0;
	pivotY = 0;
	uint32_t fill = 0;
	fill = fill | (fill << 16);
	CpuFastFill(fill, window_scanline_effect, sizeof(window_scanline_effect) / sizeof(uint32_t));

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_WIN,
		.win = {
			.win0 = WIN_DISABLE_ALL,
			.win1 = WIN_DISABLE_ALL,
			.winout = WIN_ENABLE_ALL,
			.winobj = WIN_DISABLE_ALL,
		},
	});
	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_HWREG_WINHV,
		.winhv = {
			.h = {.left = 0, .right = DISPLAY_WIDTH},
			.v = {.up = 0, .down = DISPLAY_HEIGHT},
			.to_index = 0,
		},
	});
	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_ENABLE_WIN0,
	});

	isr_switchboard_set(II_HBLANK, HBlankCB_starIris);
	isr_enable(II_HBLANK);
};

static void transition_starIris_draw1(int row, int column) {
	window_horizontal_t drawX = {
		.left = DISPLAY_WIDTH - column,
		.right = column - 1,
	};

	window_scanline_effect[row] = drawX;
	window_scanline_effect[DISPLAY_HEIGHT - 1 - row] = drawX;
}

static void transition_starIris_draw(void) {
	int pivotX = pivotY + (pivotY / 2);

	int row = 0;
	for (; row < edgeY; row++) {
		window_scanline_effect[row] = (window_horizontal_t) {DISPLAY_WIDTH, 0};
		window_scanline_effect[DISPLAY_HEIGHT - 1 - row] = (window_horizontal_t) {DISPLAY_WIDTH, 0};
	}
	for (; row < pivotY; row++) {
		int left = (row - edgeY) * (pivotX - (DISPLAY_WIDTH / 2)) / (pivotY - edgeY) + (DISPLAY_WIDTH / 2);
		transition_starIris_draw1(row, left);
	}
	for (; row < DISPLAY_HEIGHT / 2; row++) {
		int left = (pivotX - edgeX) * (row - (DISPLAY_HEIGHT / 2)) / (pivotY - (DISPLAY_HEIGHT / 2)) + edgeX;
		transition_starIris_draw1(row, left);
	}
}

static enum progress transition_starIris_out(void) {
	if (pivotY >= DISPLAY_HEIGHT / 2) {
		for (int row = 0; row < DISPLAY_HEIGHT; row++) {
			window_scanline_effect[row] = (window_horizontal_t) {DISPLAY_WIDTH, 0};
		}
		return COMPLETE;
	}

	if (pivotY < FIRST_HALF_HEIGHT) {
		pivotY += 1;
	} else {
		edgeX += 3;
		edgeY += 2;
		pivotY += 1;
	}

	transition_starIris_draw();

	return ONGOING;
};

static void transition_starIris_initIn(const union palette512* palette) {
	palette16_t* buffer = malloc(sizeof(palette16_t) * 32);
	if (buffer) {
		CpuFastCopy(
			palette->all,
			buffer,
			sizeof(palette16_t) * 32 / sizeof(uint32_t));
		buffer[0][0] = rgb(0,0,0);
		vram_op_queue_enqueue(&(struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_PALETTES_FREE,
			.palettes_free = {
				.from = buffer,
				.to_palette = 0,
				.count = 32,
			},
		});
	}

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_ENABLE_WIN0,
	});
};

static enum progress transition_starIris_in(void) {
	if (pivotY < FIRST_HALF_HEIGHT) {
		pivotY -= 1;
	} else {
		edgeX -= 3;
		edgeY -= 2;
		pivotY -= 1;
	}

	transition_starIris_draw();

	if (pivotY < 0) {
		isr_disable(II_HBLANK);
		reg_lcd.DISPCNT.enable_win0 = false;
		return COMPLETE;
	} else {
		return ONGOING;
	}
};

const struct transition transition_starIris = {
	.initFadeOut = transition_starIris_initOut,
	.fadeOut = transition_starIris_out,
	.initFadeIn = transition_starIris_initIn,
	.fadeIn = transition_starIris_in,
};
