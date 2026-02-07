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


static void bench_dmgMusicUsingNotation_init() {
	unsigned frameNo = 0;
	MgbaPrintf(MGBA_LOG_INFO, "dmgMusicUsingNotation main: \033[44mBENCH\033[0m");

	StartTransition(
		&transition_paletteFade_black,
		&(struct transitionSourceCallbacks) {0},
		&transitionTargetCbs_dmgMusicUsingNotation);

	while (transitionTargetCbs_dmgMusicUsingNotation.target != scene_onframe_callback) {
		VBlankIntrWait();

		benchmark_start();
		vram_op_queue_execute();
		uint32_t opsqueue_time = benchmark_stop();

		benchmark_start();
		scene_onframe_callback();
		uint32_t scene_time = benchmark_stop();

		if (opsqueue_time + scene_time > 1000)
		{
			const char* opsqueue_color = (opsqueue_time > CYCLES_PER_VBLANK ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "    [%3d] vram_ops: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				frameNo, opsqueue_color, opsqueue_time, opsqueue_time / CYCLES_PER_FRAME, (opsqueue_time * 1000 / CYCLES_PER_FRAME) % 1000);

			const char* scene_color = ((scene_time + opsqueue_time) > (CYCLES_PER_FRAME) ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "             scene: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				scene_color, scene_time, scene_time / CYCLES_PER_FRAME, (scene_time * 1000 / CYCLES_PER_FRAME) % 1000);
		}

		++frameNo;
	}
}


int main() {
	total = 0;
	failed = 0;

	reg_interrupt.WAITCNT = common_waitcnt;
	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	bench_dmgMusicUsingNotation_init();

	return failed != 0;
}
