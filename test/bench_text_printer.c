#include "text_printer.h"

#include "gba/hw_reg.h"
#include "management/isr.h"
#include "benchmarks.h"
#include "graphics.h"
#include "mgba.h"

static const struct shadow_tiles_window_allocate whole_screen_window = {
	.bg = 0,
	.palette = 0,
	.x = 0,
	.y = 0,
	.width = 32,
	.height = 32,
};

static const char lorem_ipsum[] =
	"Lorem ipsum dolor sit amet, consectetur\n"
	"adipiscing elit, sed do eiusmod tempor\n"
	"incididunt ut labore et dolore magna aliqua.\n"
	"Ut enim ad minim veniam, quis nostrud\n"
	"exercitation ullamco laboris nisi ut aliquip\n"
	"ex ea commodo consequat. Duis aute irure\n"
	"dolor in reprehenderit in voluptate velit\n"
	"esse cillum dolore eu fugiat nulla pariatur.\0\n"
	"Excepteur sint occaecat cupidatat non\n"
	"proident, sunt in culpa qui officia deserunt\n"
	"mollit anim id est laborum.";

void setUp(void){}
void tearDown(void){}
bool check(void){return true;}

void bench_text_printer_lorem_ipsum(void) {
	text_print_immediate(
		vram.bg_charblock[0],
		&whole_screen_window,
		&bitmapfont,
		(coord16_t) {4, 4},
		(coord16_t) {-1, 0},
		(font_colors_t) {4,1,2,3, false},
		lorem_ipsum);
}

int main() {
	reg_interrupt.WAITCNT = common_waitcnt;
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	RUN_BENCHMARK(bench_text_printer_lorem_ipsum, check);

	return 0;
}
