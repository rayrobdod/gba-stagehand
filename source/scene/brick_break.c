#include "scene/brick_break.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/screen.h"
#include "management/keyinput.h"
#include "management/scene_graphics.h"
#include "management/shadow_oam.h"
#include "management/vram_op_queue.h"
#include "brickbreak_background.png.h"
#include "graphics.h"
#include "main.h"
#include "mgba.h"
#include "saturating_add.h"

typedef struct {
	int8_t x;
	int8_t y;
} coords8_t;

// model
static ucoords16_t ballpos;
static coords8_t ballvelocity;

static bool ball_stuck_to_paddle;
static uint16_t paddle_x;
static const uint16_t paddle_y = 140;
static const unsigned paddle_x_min = 8 + 16;
static const unsigned paddle_x_max = 160 - 8 - 16;

// viewmodel
static shadow_oam_id_t spriteid_ball;
static shadow_oam_id_t spriteid_paddle;

//

//
static void MainCB_brickBreak_main(void);

void MainCB_brickBreak_init(void) {
	shadow_oam_free_all();

	ball_stuck_to_paddle = true;
	paddle_x = 80;
	ballpos = (ucoords16_t){.x = paddle_x, .y = paddle_y};
	ballvelocity = (coords8_t){.x = 1, .y = -1};

	queue_load_scene_graphics(
		&brickbreak_background,
		(struct load_scene_graphics){});

	reg_lcd.DISPCNT.enable_obj = true;

	spriteid_ball = shadow_oam_add_sprite(
		&breakout_set_ball,
		(struct shadow_oam_position){
			.coord = ballpos,
			.hotspot = HOTSPOT_CENTER,
		});

	spriteid_paddle = shadow_oam_add_sprite(
		&breakout_set_paddle,
		(struct shadow_oam_position){
			.coord = (ucoords16_t){.x = paddle_x, .y = paddle_y},
			.hotspot = HOTSPOT_TOP,
		});

	scene_onframe_callback = &MainCB_brickBreak_main;
}

static void MainCB_brickBreak_main(void) {
	bool redraw_paddle;

	int dselection = keyinput_horizontal_pressed();
	if (dselection) {
		redraw_paddle = true;
		paddle_x = saturating_add(paddle_x, paddle_x_min, paddle_x_max, dselection);
	}

	if (ball_stuck_to_paddle) {
		ballpos = (ucoords16_t){.x = paddle_x, .y = paddle_y};

		if (! keyinput_get_new().a) {
			ball_stuck_to_paddle = false;
		}
	} else {
		ballpos.x += ballvelocity.x;
		ballpos.y += ballvelocity.y;

		if (ballvelocity.x > 0 && ballpos.x >= 160 - 8 - 4) {
			ballvelocity.x = -ballvelocity.x;
		}
		if (ballvelocity.x < 0 && ballpos.x <= 8 + 4) {
			ballvelocity.x = -ballvelocity.x;
		}
		if (ballvelocity.y < 0 && ballpos.y <= 8 + 4) {
			ballvelocity.y = -ballvelocity.y;
		}
		if (ballvelocity.y > 0 && ballpos.y >= 160 + 4) {
			ballvelocity.y = -ballvelocity.y;
		}
	}

	if (redraw_paddle) {
		shadow_oam_move_sprite(
			spriteid_paddle,
			(struct shadow_oam_position){
				.coord = (ucoords16_t){.x = paddle_x, .y = paddle_y},
				.hotspot = HOTSPOT_TOP,
			});
	}

	shadow_oam_move_sprite(
		spriteid_ball,
		(struct shadow_oam_position){
			.coord = ballpos,
			.hotspot = HOTSPOT_CENTER,
		});
}
