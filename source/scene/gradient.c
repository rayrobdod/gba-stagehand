#include "scene/gradient.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "gba/bios.h"
#include "gba/palette.h"
#include "gba/screen.h"
#include "management/isr.h"
#include "management/keyinput.h"
#include "management/vram_op_queue.h"
#include "scene/main_menu.h"
#include "utils/minmax.h"
#include "main.h"

static void InitFadeOut_gradient(void);
static enum progress FadeOut_gradient(void);
static union palette512 InitFadeIn_gradient(void);
static void MainCB_gradient(void);
static void CleanCB_gradient(void);

static const struct transitionSourceCallbacks transitionSourceCbs_gradient = {
	.fadeOut = NULL,
	.cleanup = CleanCB_gradient,
};
const struct transitionTargetCallbacks transitionTargetCbs_gradient = {
	.initFadeOut = InitFadeOut_gradient,
	.fadeOut = FadeOut_gradient,
	.initFadeIn = InitFadeIn_gradient,
	.fadeIn = NULL,
	.target = MainCB_gradient,
};

static uint8_t row_color_index = {0};
static rgb_t* row_colors = {0};

static rgb_t hsb2rgb(int /* [0, 6 * 256] fixpoint 256 */ hue, int /* [0, 31] */ saturation, unsigned /* [0, 31] */ brightness) {
	int /* [0, 31] */ r, g, b;
	int /* [0, 6 * 256] fixpoint 256 */ k;
	int /* [0, 256] fixpoint 256 */ y;
	int z;

	k = ((5 * 256) + hue) % (6 * 256);
	y = max(0, min(256, min(k, 4 * 256 - k)));
	z = brightness * ((31 * 256) - (saturation * y));
	r = z / (256 * 31);

	k = ((3 * 256) + hue) % (6 * 256);
	y = max(0, min(256, min(k, 4 * 256 - k)));
	z = brightness * ((31 * 256) - (saturation * y));
	g = z / (256 * 31);

	k = ((1 * 256) + hue) % (6 * 256);
	y = max(0, min(256, min(k, 4 * 256 - k)));
	z = brightness * ((31 * 256) - (saturation * y));
	b = z / (256 * 31);

	return rgb(r, g, b);
}

static void HblankCB_gradient(void) {
	uint16_t row = reg_lcd.VCOUNT;
	if (row < DISPLAY_HEIGHT) {
		hw_palette.background._4[0][1] = row_colors[row];
	}
}

static void InitFadeOut_gradient(void) {
	row_colors = malloc(DISPLAY_HEIGHT * sizeof(rgb_t));
	row_color_index = 0;
}

static enum progress FadeOut_gradient(void) {
	if (! row_colors) {
		return COMPLETE;
	}

	const int scanlines_left = (
		reg_lcd.VCOUNT < DISPLAY_HEIGHT ?
		DISPLAY_HEIGHT - reg_lcd.VCOUNT :
		VCOUNT_MAX + DISPLAY_HEIGHT - reg_lcd.VCOUNT) - 1;
	const int max_rgbs = scanlines_left * 42 / 100;

	const int segment_length = min(max_rgbs, DISPLAY_HEIGHT - row_color_index);

	for (int i = 0; i < segment_length; i++) {
		row_colors[row_color_index + i] = hsb2rgb((row_color_index + i) * 6 * 256 / DISPLAY_HEIGHT, 31, 31);
	}

	row_color_index += segment_length;

	return (row_color_index == DISPLAY_HEIGHT ? COMPLETE : ONGOING);
}

static union palette512 InitFadeIn_gradient(void) {
	if (row_colors) {
		hw_palette.background._4[0][1] = row_colors[0];
		isr_switchboard_set(II_HBLANK, &HblankCB_gradient);
		isr_enable(II_HBLANK);
	}

	vram_op_queue_enqueue(&(struct vram_op) {
		.type = VRAM_QUEUE_OP_DISABLE_ALL_OAM,
	});

	union palette512 retval = {0};
	retval.background._4[0][1] = rgb(31,31,31);
	return retval;
}

static void MainCB_gradient(void) {
	if (! keyinput_get_down().b) {
		StartTransition(
			&transition_cut,
			&transitionSourceCbs_gradient,
			&transitionTargetCbs_mainMenu);
	}
}

static void CleanCB_gradient(void) {
	isr_disable(II_HBLANK);
	free(row_colors);
	row_colors = NULL;
}
