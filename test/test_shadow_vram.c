#include "management/shadow_vram.h"

#include <stdlib.h>
#include "harness.h"
#include "mgba.h"
#include "graphics.h"
#include "graphics_types.h"
#include "management/vram_op_queue.h"
#include "decompress/by_header.h"
#include "decompress/type.h"

static const struct shadow_vram_init normal_shadow_vram_init = {
	.enable_bg = {true, false, false, false},
	.enable_obj = false,
	.enable_win = {false, false, false},
	.bgcnt = {
		[0] = {
			.charblock = 0,
			.screenblock = 20,
		},
	},
};

void setUp(void) {
	vram = (union vram) {0};
	shadow_vram_init(&normal_shadow_vram_init);
}
void tearDown(void){}

void test_load_background__happy(void) {
	MgbaPrintf(MGBA_LOG_INFO, "{%p, %d, %x, %p (%02x), %d, %x, %p (%02x), %d}",
		brickbreak_background.tileset.palette,
		brickbreak_background.tileset.palette_count,
		brickbreak_background.tileset.paltag,
		brickbreak_background.tileset.tileset,
		brickbreak_background.tileset.tileset->magic,
		brickbreak_background.tileset.tileset_count,
		brickbreak_background.tileset.tiletag,
		brickbreak_background.tilemap,
		brickbreak_background.tilemap->magic,
		brickbreak_background.tilemap_count);

	bool retval = shadow_tiles_load_background(
		&brickbreak_background,
		(struct shadow_tiles_load_background) {0}
	);

	TEST_ASSERT(retval, "load op failed");

	vram_op_queue_execute();

	uint16_t* tileset_raw = malloc(((brickbreak_background.tileset.tileset->size / 2) + 1) * 2);
	HeaderUnCompWram(brickbreak_background.tileset.tileset, tileset_raw);

	uint16_t* tilemap_raw = malloc(((brickbreak_background.tilemap->size / 2) + 1) * 2);
	HeaderUnCompWram(brickbreak_background.tilemap, tilemap_raw);

	TEST_ASSERT_EQUAL_UINT16_ARRAY(
		(uint16_t*) brickbreak_background.tileset.palette,
		(uint16_t*) hw_palette.background._4[0],
		16 * brickbreak_background.tileset.palette_count);

	TEST_ASSERT_EQUAL_UINT16_ARRAY(
		tileset_raw,
		(uint16_t*) vram.bg_charblock[0][0],
		32 / 2 * brickbreak_background.tileset.tileset_count);

	TEST_ASSERT_EQUAL_UINT16_ARRAY(
		tilemap_raw,
		(uint16_t*) vram.screenblock[20],
		brickbreak_background.tilemap_count);

	free(tileset_raw);
	free(tilemap_raw);
}

int main() {
	total = 0;
	failed = 0;

	MgbaOpen();

	RUN_TEST(test_load_background__happy);

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return 0 != failed;
}
