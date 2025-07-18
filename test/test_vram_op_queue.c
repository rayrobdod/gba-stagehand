#include "management/vram_op_queue.h"

#include <stdio.h>
#include <string.h>
#include "utils/arraycount.h"
#include "harness.h"
#include "mgba.h"

void setUp(void){}
void tearDown(void){}

static const rgb_t black = {0,0,0};
static const palette16_t rainbow = {{0,0,0}, {31,0,0}, {31,16,0}, {31,31,0}, {0,31,0}, {0,31,31}, {0,0,31}, {31,0,31}};

void test_vram_op_queue_enqueue_bg_palette(void) {
	for (unsigned i = 0; i < arraycount(rainbow); i++) {
		background_palette[1][i] = black;
	}
	vram_op_queue_enqueue(
		(struct vram_op) {
			.type = VRAM_QUEUE_OP_BG_PALETTES,
			.palettes = {
				.from = &rainbow,
				.to_palette = 1,
				.count = 1,
			}
		}
	);
	for (unsigned i = 0; i < arraycount(rainbow); i++) {
		TEST_ASSERT_EQUAL_RGB(black, background_palette[1][i]);
	}
	vram_op_queue_execute();
	for (unsigned i = 0; i < arraycount(rainbow); i++) {
		TEST_ASSERT_EQUAL_RGB(rainbow[i], background_palette[1][i]);
	}
}

int main() {
	total = 0;
	failed = 0;

	MgbaOpen();

	RUN_TEST(test_vram_op_queue_enqueue_bg_palette);

	MgbaPrintf(MGBA_LOG_INFO, "Total: %d; Failing: %d", total, failed);
	return 0 != failed;
}
