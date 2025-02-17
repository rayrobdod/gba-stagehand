#include "scene/brick_break.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "management/keyinput.h"
#include "management/scene_graphics.h"
#include "management/shadow_oam.h"
#include "management/vram_op_queue.h"
#include "brickbreak_background.png.h"
#include "main.h"
#include "mgba.h"

// model

// viewmodel

//

//
static void MainCB_brickBreak_main(void);

void MainCB_brickBreak_init(void) {
	shadow_oam_free_all();

	reg_lcd.DISPCNT.enable_obj = true;

	queue_load_scene_graphics(
		&brickbreak_background,
		(struct load_scene_graphics){});

	scene_onframe_callback = &MainCB_brickBreak_main;
}

static void MainCB_brickBreak_main(void) {
}
