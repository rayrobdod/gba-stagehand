#include "scene/brick_break.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gba/bios.h"
#include "gba/screen.h"
#include "management/keyinput.h"
#include "management/scene_graphics.h"
#include "management/shadow_oam.h"
#include "management/shadow_vram.h"
#include "management/vram_op_queue.h"
#include "graphics.h"
#include "main.h"
#include "mgba.h"
#include "saturating_add.h"

#define arraycount(a) (sizeof(a) / sizeof(a[0]))

enum {
	BALLPOS_SCALE = 1 << 6,
};

typedef struct {
	int8_t x;
	int8_t y;
} coords8_t;

/** Converts the fractional part of a BALLPOS fixpoint to a three-digit string */
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

static const tile_4bpp_t tile_zero = {0};

inline static void MgbaPrintBallposFixpoint(char* var_name, int value) {
	MgbaPrintf(MGBA_LOG_INFO, "%s: %d (%d.%s)", var_name, value, value / BALLPOS_SCALE, ballpos_scale_frac[abs(value % BALLPOS_SCALE)]);
}

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

__attribute__((section(".sbss")))
static struct {
	ucoords16_t pos;
	uint16_t health;
} bricks[64];

static const unsigned brick_halfwidth = 10 * BALLPOS_SCALE;
static const unsigned brick_halfheight = 6 * BALLPOS_SCALE;

// viewmodel
static shadow_oam_id_t spriteid_ball;
static shadow_oam_id_t spriteid_paddle;
static shadow_oam_id_t spriteid_bricks[64];
__attribute__((section(".sbss")))
static bg_tile_t zero_tile_ref;

static uint8_t paddle_skin;
static const struct shadow_oam_template* paddle_skins[] = {
	&breakout_set_paddle,
	&breakout_set_paddle_green,
	&breakout_set_paddle_red,
	&breakout_set_paddle_purple,
	&breakout_set_paddle_yellow,
	&breakout_set_paddle_grey,
	&breakout_set_paddle_brown,
};
static const struct shadow_oam_template* brick_skins[] = {
	&breakout_set_brick,
	&breakout_set_brick_green,
	&breakout_set_brick_red,
	&breakout_set_brick_purple,
	&breakout_set_brick_yellow,
	&breakout_set_brick_grey,
	&breakout_set_brick_brown,
};

//

inline static bool is_between(int value, int left, int right) {
	return left < value && value < right;
}
inline static bool is_between_offset(int value, int center, int offset) {
	return is_between(value, center - offset, center + offset);
}
inline static ucoords16_t coord_add(ucoords16_t a, coords8_t b) {
	return (ucoords16_t) {
		.x = a.x + b.x,
		.y = a.y + b.y,
	};
}
inline static bool point_in_brick(ucoords16_t ballpos, ucoords16_t brickpos) {
	return is_between_offset(ballpos.x, brickpos.x, brick_halfwidth) &&
		is_between_offset(ballpos.y, brickpos.y, brick_halfheight);
}
inline static bool ball_hit_brick_collision(ucoords16_t ballpos, coords8_t ballvelocity, ucoords16_t brickpos) {
	return ! point_in_brick(ballpos, brickpos) &&
		point_in_brick(coord_add(ballpos, ballvelocity), brickpos);
}

//
static void MainCB_brickBreak_main(void);

void MainCB_brickBreak_init(void) {
	reg_lcd.DISPCNT = (dispcnt_t) {
		.mode = 0,
		.obj_character_mapping = OBJ_CHAR_MAP_1D,
		.enable_bg0 = true,
		.enable_bg3 = true,
	};
	reg_lcd.BGCNT[0] = (bgcnt_t) {
		.priority = 3,
		.charblock = 0,
		.screenblock = 31,
	};
	reg_lcd.BGCNT[3] = (bgcnt_t) {
		.priority = 0,
		.charblock = 0,
		.screenblock = 30,
	};

	shadow_oam_init();
	shadow_vram_init();

	paddle_skin = 0;
	ball_stuck_to_paddle = true;
	paddle_x = 80;
	ballpos = (ucoords16_t){.x = paddle_x * BALLPOS_SCALE, .y = paddle_y * BALLPOS_SCALE};
	ballvelocity = (coords8_t){.x = BALLPOS_SCALE, .y = -BALLPOS_SCALE};

	shadow_tiles_load_background(
		&brickbreak_background,
		(struct shadow_tiles_load_background){.bg = 0});

	zero_tile_ref = (bg_tile_t) {.tile = shadow_tiles_load_tileset(3, 1, &tile_zero)};

	vram_op_queue_enqueue((struct vram_op) {
		.type = VRAM_QUEUE_OP_BG_MAP_FILL,
		.map_fill = {
			.value = zero_tile_ref,
			.to_block = 30,
			.to_tile = 0,
			.count = 32 * 20,
		},
	});

	reg_lcd.DISPCNT.enable_obj = true;

	for (unsigned i = 0; i < arraycount(paddle_skins); i++) {
		shadow_oam_preload_sprite(paddle_skins[i]);
	}

	spriteid_ball = shadow_oam_add_sprite(
		&breakout_set_ball_green,
		(struct shadow_oam_position){
			.coord = (ucoords16_t){.x = ballpos.x / BALLPOS_SCALE, .y = ballpos.y / BALLPOS_SCALE},
			.hotspot = HOTSPOT_CENTER,
		});

	spriteid_paddle = shadow_oam_add_sprite(
		paddle_skins[paddle_skin],
		(struct shadow_oam_position){
			.coord = (ucoords16_t){.x = paddle_x, .y = paddle_y},
			.hotspot = HOTSPOT_TOP,
		});

	for (unsigned i = 0; i < 64; i++) {
		uint16_t x = 24 + 16 * (i % 8);
		uint16_t y = 16 + 8 * (i / 8);

		spriteid_bricks[i] = shadow_oam_add_sprite(
			brick_skins[2],
			(struct shadow_oam_position){
				.coord = (ucoords16_t){.x = x, .y = y},
				.hotspot = HOTSPOT_CENTER,
			});
		bricks[i].health = 1;
		bricks[i].pos.x = x * BALLPOS_SCALE;
		bricks[i].pos.y = y * BALLPOS_SCALE;
	}

	scene_onframe_callback = &MainCB_brickBreak_main;
}

static void MainCB_brickBreak_main(void) {
	bool redraw_paddle;

	int dselection = keyinput_horizontal_pressed();
	if (dselection) {
		redraw_paddle = true;
		paddle_x = saturating_add(paddle_x, paddle_x_min, paddle_x_max, dselection);
	}

	int dshoulder = keyinput_shoulders_new();
	if (dshoulder) {
		redraw_paddle = false;
		paddle_skin = (paddle_skin + dshoulder + arraycount(paddle_skins)) % arraycount(paddle_skins);

		shadow_oam_remove_sprite(spriteid_paddle);
		spriteid_paddle = shadow_oam_add_sprite(
			paddle_skins[paddle_skin],
			(struct shadow_oam_position){
				.coord = (ucoords16_t){.x = paddle_x, .y = paddle_y},
				.hotspot = HOTSPOT_TOP,
			});
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
		for (int i = 0; i < arraycount(bricks); i++) {
			if (0 == bricks[i].health) {
				continue;
			}

			bool is_hit = ball_hit_brick_collision(ballpos, ballvelocity, bricks[i].pos);

			if (is_hit) {
				if (! is_between_offset(ballpos.x, bricks[i].pos.x, brick_halfwidth)) {
					ballvelocity.x = -ballvelocity.x;
				}
				if (! is_between_offset(ballpos.y, bricks[i].pos.y, brick_halfheight)) {
					ballvelocity.y = -ballvelocity.y;
				}

				bricks[i].health--;
				if (0 == bricks[i].health) {
					shadow_oam_remove_sprite(spriteid_bricks[i]);
					spriteid_bricks[i] = 0xFF;
				}
				break;
			}
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

			// if neither dx nor dy can be larger than 16,
			// then hypotSq should be no larger than 512.
			// 512 is 10 bits; BALLPOS_SCALE is 6 bits, squared is 12 bits.
			// A 10.12 fixpoint fits in the 32 bits available to an int
			const int hypotSq = (dx * dx / BALLPOS_SCALE) + (dy * dy / BALLPOS_SCALE);
			const int hypot = Sqrt(hypotSq * BALLPOS_SCALE);
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
