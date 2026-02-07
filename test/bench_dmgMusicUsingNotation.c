#include "scene/dmg_music_using_notation.h"

#include <stddef.h>
#include <stdio.h>
#include "management/isr.h"
#include "management/keyinput.h"
#include "gba/bios.h"
#include "management/vram_op_queue.h"
#include "transition/palette_fade.h"
#include "benchmarks.h"
#include "main.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;

MainCallback scene_onframe_callback;

void transitionTargetCbs_mainMenu_initFadeOut(void) {
	MgbaPrintf(MGBA_LOG_INFO, "ENTER: transitionTargetCbs_mainMenu");
	asm(
		"movs	r0,	#1\n\t"
		"swi	#0x00"
	);
}
const struct transitionTargetCallbacks transitionTargetCbs_mainMenu = {
	transitionTargetCbs_mainMenu_initFadeOut,
	NULL,
	NULL,
	NULL,
	NULL,
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
		&transition_paletteFade_black,
		&(struct transitionSourceCallbacks) {0},
		&transitionTargetCbs_dmgMusicUsingNotation,
		"dmgMusicUsingNotation init",
		RTBV_SUMMARY_ONLY);

	return failed != 0;
}
