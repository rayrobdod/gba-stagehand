#include "scene/dmg_music_using_notation.h"

#include <stddef.h>
#include <stdio.h>
#include "gba/bios.h"
#include "management/isr.h"
#include "management/keyinput.h"
#include "management/vram_op_queue.h"
#include "transition/cut.h"
#include "transition/palette_fade.h"
#include "transition/star_iris.h"
#include "benchmarks.h"
#include "main.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;

MainCallback scene_onframe_callback;

union palette512 InitFadeIn_null(void) {
	union palette512 retval = {0};
	return retval;
}
void MainCB_null(void){}
const struct transitionTargetCallbacks transitionTargetCbs_null = {
	.initFadeOut = NULL,
	.fadeOut = NULL,
	.initFadeIn = InitFadeIn_null,
	.fadeIn = NULL,
	.target = MainCB_null,
};
const struct transitionSourceCallbacks transitionSourceCbs_null = {
	.fadeOut = NULL,
	.cleanup = NULL,
};

void setUp(void){}
void tearDown(void){}


int main() {
	total = 0;
	failed = 0;

	reg_interrupt.WAITCNT = common_waitcnt;
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	run_transition_benchmark(
		&transition_starIris,
		&transitionSourceCbs_null,
		&transitionTargetCbs_null,
		"starIris",
		RTBV_SUMMARY_ONLY);

	run_transition_benchmark(
		&transition_paletteFade_coral,
		&transitionSourceCbs_null,
		&transitionTargetCbs_null,
		"paletteFade_coral",
		RTBV_SUMMARY_ONLY);

	return failed != 0;
}
