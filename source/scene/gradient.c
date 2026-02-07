#include "scene/gradient.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "gba/bios.h"
#include "gba/palette.h"
#include "gba/screen.h"
#include "management/isr.h"
#include "management/keyinput.h"
#include "scene/main_menu.h"
#include "utils/minmax.h"
#include "main.h"

static void MainCB_gradient_main(void);
static void MainCB_gradient_clean(void);

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

void MainCB_gradient_init(void) {
	row_colors = malloc(DISPLAY_HEIGHT * sizeof(rgb_t));
	if (row_colors) {
		for (int i = 0; i < DISPLAY_HEIGHT; i++) {
			row_colors[i] = hsb2rgb(i * 6 * 256 / DISPLAY_HEIGHT, 31, 31);
		}

		VBlankIntrWait();
		isr_switchboard_set(II_HBLANK, &HblankCB_gradient);
		isr_enable(II_HBLANK);
	}

	scene_onframe_callback = &MainCB_gradient_main;
}

static void MainCB_gradient_main(void) {
	if (! keyinput_get_down().b) {
		scene_onframe_callback = &MainCB_gradient_clean;
	}
}

static void MainCB_gradient_clean(void) {
	isr_disable(II_HBLANK);
	free(row_colors);
	row_colors = NULL;
	scene_onframe_callback = &MainCB_mainMenu_init;
}
