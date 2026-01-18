#include "scene/walkaround.h"
#include "scene/walkaround_intern.h"

#include <stdio.h>
#include <string.h>
#include "utils/arraycount.h"
#include "management/vram_op_queue.h"
#include "harness.h"
#include "mgba.h"

void setUp(void){}
void tearDown(void){}
#ifdef __unix__
void vram_op_queue_enqueue([[maybe_unused]] const struct vram_op new_op) {}
#endif

void TEST_ASSERT_EQUAL_MODEL(struct walkaround_model* expected, struct walkaround_model* actual) {
	if (expected->map != actual->map ||
			expected->player.facing != actual->player.facing ||
			expected->player.turn_timer != actual->player.turn_timer ||
			expected->player.action != actual->player.action ||
			expected->player.pos.x != actual->player.pos.x ||
			expected->player.pos.y != actual->player.pos.y) {
		char msg[256];
		snprintf(msg, arraycount(msg), "Expected {%p,{{%d,%d},%d,%d,%d}}; was {%p,{{%d,%d},%d,%d,%d}}",
			expected->map,
			expected->player.pos.x, expected->player.pos.y,
			expected->player.turn_timer,
			expected->player.action,
			expected->player.facing,
			actual->map,
			actual->player.pos.x, actual->player.pos.y,
			actual->player.turn_timer,
			actual->player.action,
			actual->player.facing);
		TEST_FAIL(msg);
	}
}

void PRINT_VIEWMODEL() {
	MgbaPrintf(MGBA_LOG_INFO, "{{{%d,%d},{%d,%d}},{%d,{%d,%d},%p,%d,%d}}",
		walkaround_viewmodel.camera.bgofs.h,
		walkaround_viewmodel.camera.bgofs.v,
		walkaround_viewmodel.camera.mapoffs.x,
		walkaround_viewmodel.camera.mapoffs.y,
		walkaround_viewmodel.player.oam_id,
		walkaround_viewmodel.player.mapoffs.x,
		walkaround_viewmodel.player.mapoffs.y,
		walkaround_viewmodel.player.anim,
		walkaround_viewmodel.player.anim_frame,
		walkaround_viewmodel.player.anim_delay);
}


static const keypad_t KEYPAD_NONE = {
	.a = true,
	.b = true,
	.select = true,
	.start = true,
	.right = true,
	.left = true,
	.up = true,
	.down = true,
	.r = true,
	.l = true,
};
static const keypad_t KEYPAD_DOWN = {
	.a = true,
	.b = true,
	.select = true,
	.start = true,
	.right = true,
	.left = true,
	.up = true,
	.down = false,
	.r = true,
	.l = true,
};

static keypad_t keypad_current_down = KEYPAD_NONE;
keypad_t keyinput_get_down(void) {
	return keypad_current_down;
}

static const struct tile16x3 clear_tile16x3[] = {
	{WB_NORMAL, {{{0},{0},{0},{0}},{{0},{0},{0},{0}},{{0},{0},{0},{0}}}},
};
static const struct tile16x3map clear_map = {
	.tileset = {
		.palette = NULL,
		.tileset = NULL,
		.tileset_count = 0,
	},
	.metatileset = clear_tile16x3,
	.width = 3,
	.height = 3,
	.metatilemap = {0,0,0,0,0,0,0,0,0,0},
};

void test_walkaround__move_down(void) {
	walkaround_state = (struct walkaround_model) {
		.map = &clear_map,
		.player = {
			.pos = {1, 1},
			.turn_timer = 0,
			.action = ACTION_NONE,
			.facing = DIRECTION_SOUTH,
		},
	};
	walkaround_viewmodel = (struct walkaround_viewmodel) {
		.camera = {
			.mapoffs = {-6*16, -4*16},
		},
		.player = {
			.oam_id = shadow_id_invalid,
			.mapoffs = {1*16+8, 1*16+14},
		},
	};

	keypad_current_down = KEYPAD_DOWN;
	MainCB_walkaround_main();

	TEST_ASSERT_EQUAL_MODEL(
		&(struct walkaround_model) {
			.map = &clear_map,
			.player = {
				.pos = {1, 2},
				.turn_timer = 0,
				.action = ACTION_WALKING,
				.facing = DIRECTION_SOUTH,
			},
		},
		&walkaround_state);
}

void test_walkaround__turn_down(void) {
	walkaround_state = (struct walkaround_model) {
		.map = &clear_map,
		.player = {
			.pos = {1, 1},
			.turn_timer = 0,
			.action = ACTION_NONE,
			.facing = DIRECTION_WEST,
		},
	};
	walkaround_viewmodel = (struct walkaround_viewmodel) {
		.camera = {
			.mapoffs = {-6*16, -4*16},
		},
		.player = {
			.oam_id = shadow_id_invalid,
			.mapoffs = {1*16+8, 1*16+14},
		},
	};

	keypad_current_down = KEYPAD_DOWN;
	MainCB_walkaround_main();

	TEST_ASSERT_EQUAL_MODEL(
		&(struct walkaround_model) {
			.map = &clear_map,
			.player = {
				.pos = {1, 1},
				.turn_timer = 4,
				.action = ACTION_TURNING,
				.facing = DIRECTION_SOUTH,
			},
		},
		&walkaround_state);
}

int main() {
	total = 0;
	failed = 0;

	MgbaOpen();

	RUN_TEST(test_walkaround__move_down);
	RUN_TEST(test_walkaround__turn_down);

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return 0 != failed;
}
