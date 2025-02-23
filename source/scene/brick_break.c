#include "scene/brick_break.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/bios.h"
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

enum {
	BALLPOS_SCALE_SQRT = 1 << 3,
	BALLPOS_SCALE = 1 << 6,
};

static const char ballpos_scale_frac[BALLPOS_SCALE][4] = {
	"000", "016", "031", "047", "063", "078", "094", "109",
	"125", "141", "156", "172", "188", "203", "219", "234",
	"250", "266", "281", "297", "313", "328", "344", "359",
	"375", "391", "406", "422", "438", "453", "469", "484",
	"500", "516", "531", "547", "563", "578", "594", "609",
	"625", "641", "656", "672", "688", "703", "719", "734",
	"750", "766", "781", "797", "813", "828", "844", "859",
	"875", "891", "906", "922", "938", "953", "969", "984",
};

inline static void MgbaPrintBallposFixpoint(char* var_name, int value) {
	MgbaPrintf(MGBA_LOG_INFO, "%s: %d (%d.%s)", var_name, value, value / BALLPOS_SCALE, ballpos_scale_frac[abs(value % BALLPOS_SCALE)]);
}

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
static const unsigned paddle_height = 16;
static const unsigned paddle_width = 32;
static const unsigned paddle_x_min = 8 + paddle_width / 2;
static const unsigned paddle_x_max = 160 - 8 - paddle_width / 2;

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
	ballpos = (ucoords16_t){.x = paddle_x * BALLPOS_SCALE, .y = paddle_y * BALLPOS_SCALE};
	ballvelocity = (coords8_t){.x = BALLPOS_SCALE, .y = -BALLPOS_SCALE};

	queue_load_scene_graphics(
		&brickbreak_background,
		(struct load_scene_graphics){});

	reg_lcd.DISPCNT.enable_obj = true;

	spriteid_ball = shadow_oam_add_sprite(
		&breakout_set_ball,
		(struct shadow_oam_position){
			.coord = (ucoords16_t){.x = ballpos.x / BALLPOS_SCALE, .y = ballpos.y / BALLPOS_SCALE},
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
		ballpos = (ucoords16_t){.x = paddle_x * BALLPOS_SCALE, .y = paddle_y * BALLPOS_SCALE};

		if (! keyinput_get_new().a) {
			ball_stuck_to_paddle = false;
		}
	} else {
		ballpos.x += ballvelocity.x;
		ballpos.y += ballvelocity.y;

		if (ballvelocity.x > 0 && ballpos.x >= (160 - 8 - 4) * BALLPOS_SCALE) {
			ballvelocity.x = -ballvelocity.x;
		}
		if (ballvelocity.x < 0 && ballpos.x <= (8 + 4) * BALLPOS_SCALE) {
			ballvelocity.x = -ballvelocity.x;
		}
		if (ballvelocity.y < 0 && ballpos.y <= (8 + 4) * BALLPOS_SCALE) {
			ballvelocity.y = -ballvelocity.y;
		}
		if (ballvelocity.y > 0 && ballpos.y >= (160 + 4) * BALLPOS_SCALE) {
			ballvelocity.y = -ballvelocity.y;
		}
		if (
			ballpos.y > (paddle_y * BALLPOS_SCALE) &&
			ballpos.y < (paddle_y + paddle_height) * BALLPOS_SCALE &&
			ballpos.x + (paddle_width / 2 - paddle_x) * BALLPOS_SCALE < paddle_width * BALLPOS_SCALE
		) {
			const int dx = (ballpos.x - (paddle_x * BALLPOS_SCALE));
			const int dy = (ballpos.y - ((paddle_y + paddle_height / 2) * BALLPOS_SCALE));
			if (false) {
				MgbaPrintBallposFixpoint("dx", dx);
				MgbaPrintBallposFixpoint("dy", dy);
			}

			const int hypotSq = (dx * dx / BALLPOS_SCALE) + (dy * dy / BALLPOS_SCALE);
			const int hypot = Sqrt(hypotSq) * BALLPOS_SCALE_SQRT;
			if (false) {
				MgbaPrintBallposFixpoint("hypot", hypot);
			}

			if (hypot) {
				ballvelocity.x = (dx * BALLPOS_SCALE * 3 / 2) / hypot;
				ballvelocity.y = (dy * BALLPOS_SCALE * 3 / 2) / hypot;
			}
			if (0 == ballvelocity.y)
				ballvelocity.y = -1;

			if (false) {
				MgbaPrintBallposFixpoint("ball dx", ballvelocity.x);
				MgbaPrintBallposFixpoint("ball dy", ballvelocity.y);
			}
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
			.coord = (ucoords16_t){.x = ballpos.x / BALLPOS_SCALE, .y = ballpos.y / BALLPOS_SCALE},
			.hotspot = HOTSPOT_CENTER,
		});
}
