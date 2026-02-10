#include "scene/walkaround.h"
#include "scene/walkaround_intern.h"

#include <stddef.h>
#include <stdio.h>
#include "gba/bios.h"
#include "management/isr.h"
#include "management/keyinput.h"
#include "management/transition.h"
#include "management/vram_op_queue.h"
#include "transition/palette_fade.h"
#include "benchmarks.h"
#include "main.h"
#include "mgba.h"

static unsigned total;
static unsigned failed;

MainCallback scene_onframe_callback;

void ChangeScene_options(...) {
	MgbaPrintf(MGBA_LOG_INFO, "ENTER: ChangeScene_options");
	asm(
		"movs	r0,	#1\n\t"
		"swi	#0x00"
	);
}


void setUp(void){}
void tearDown(void){}



static void run_walkaround_idle_benchmark() {
	MgbaPrintf(MGBA_LOG_INFO, "walkaround idle: \033[44mBENCH\033[0m");

	StartTransition(
		&transition_paletteFade_black,
		&(struct transitionSourceCallbacks) {0},
		&transitionTargetCbs_walkaround_newgame);

	while (MainCB_walkaround != scene_onframe_callback) {
		vram_op_queue_execute();
		scene_onframe_callback();
	}

	for (unsigned frameNo = 0; frameNo < 50; ++frameNo) {
		VBlankIntrWait();

		benchmark_start();
		vram_op_queue_execute();
		uint32_t opsqueue_time = benchmark_stop();

		benchmark_start();
		scene_onframe_callback();
		uint32_t scene_time = benchmark_stop();

		if (opsqueue_time + scene_time > 10000)
		{
			const char* opsqueue_color = (opsqueue_time > CYCLES_PER_VBLANK ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "    [%3d] vram_ops: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				frameNo, opsqueue_color, opsqueue_time, opsqueue_time / CYCLES_PER_FRAME, (opsqueue_time * 1000 / CYCLES_PER_FRAME) % 1000);

			const char* scene_color = ((scene_time + opsqueue_time) > (CYCLES_PER_FRAME) ? "\033[43m" : "\033[0m");
			MgbaPrintf(MGBA_LOG_INFO, "             scene: %s%8ld cycles = %2ld.%03ld frames\033[0m",
				scene_color, scene_time, scene_time / CYCLES_PER_FRAME, (scene_time * 1000 / CYCLES_PER_FRAME) % 1000);
		}
	}
}


int main() {
	total = 0;
	failed = 0;

	isr_switchboard_init();
	isr_enable(II_VBLANK);
	MgbaOpen();

	run_transition_benchmark(
		&transition_paletteFade_black,
		&(struct transitionSourceCallbacks) {0},
		&transitionTargetCbs_walkaround_newgame,
		"walkaround init",
		RTBV_ALL_FRAMES);

	run_walkaround_idle_benchmark();

	return failed != 0;
}
